#include <std/std.hpp>
#include "SkrRenderer/shared/gpu_scene.hpp"

ByteAddressBuffer gpu_insts;
ByteAddressBuffer gpu_mats;
ByteAddressBuffer gpu_prims;

RWTexture2D<float> output_texture;
Accel scene_tlas;

// Push constants - camera parameters
struct CameraConstants 
{
    float4 cameraPos;
    float4 cameraDir;
    float2 screenSize;
};

[[push_constant]]
ConstantBuffer<CameraConstants> camera_constants;

ByteAddressBuffer geom_buffers[0];
Texture2D<> mat_textures[0];

[[group(1)]] SamplerState tex_sampler;

// Ray tracing constants
trait RayTracingConstants {
    static constexpr uint32 MAX_BOUNCES = 1;
    static constexpr float EPSILON = 0.001f;
    static constexpr float MAX_DISTANCE = 100000.0f;
};

// Generate camera ray from screen coordinates
Ray generate_camera_ray(uint2 pixel_coord, uint2 screen_size) 
{
    // Screen coordinates to normalized device coordinates [-1, 1]
    float2 ndc = float2(
        (float(pixel_coord.x) + 0.5f) / float(screen_size.x) * 2.0f - 1.0f,
        1.0f - (float(pixel_coord.y) + 0.5f) / float(screen_size.y) * 2.0f
    );
    
    // Simplified approach: directly compute ray direction from camera
    float3 ray_origin = camera_constants.cameraPos.xyz;
    
    // Camera setup: eye=(0,500,-200), target=(0,0,-800), up=(0,1,0)
    float3 camera_forward = normalize(camera_constants.cameraDir.xyz);
    float3 camera_right = normalize(cross(camera_forward, float3(0, 1, 0)));
    float3 camera_up = cross(camera_right, camera_forward);
    
    // Simple perspective projection
    float fov_scale = tan(45.0f * 3.14159f / 180.0f / 2.0f); // 45 degree FOV
    float aspect = float(screen_size.x) / float(screen_size.y);
    
    float3 ray_direction = normalize(
        camera_forward + 
        camera_right * ndc.x * fov_scale * aspect + 
        camera_up * ndc.y * fov_scale
    );
    
    return Ray(ray_origin, ray_direction, RayTracingConstants::EPSILON, RayTracingConstants::MAX_DISTANCE);
}

// Colorful sphere shading based on instance ID
float4 shade_hit(RayQuery<RayQueryFlags::AcceptFirstAndEndSearch>& query) 
{
    uint instance_id = query.CommittedInstanceID();
    const auto instance_row = skr::gpu::Row<skr::gpu::Instance>(instance_id); 
    const auto instance = instance_row.Load(gpu_insts);
    const auto prim = instance.primitives.LoadAt(gpu_prims, query.CommittedGeometryIndex());
    const auto mat = instance.materials.LoadAt(gpu_mats, prim.material_index);
    const auto tri = prim.triangles.LoadAt(geom_buffers, query.CommittedPrimitiveIndex());
    const auto uv_a = prim.uvs.LoadAt(geom_buffers, tri[0]);
    const auto uv_b = prim.uvs.LoadAt(geom_buffers, tri[1]);
    const auto uv_c = prim.uvs.LoadAt(geom_buffers, tri[2]);
    const auto uv = interpolate(query.CommittedTriangleBarycentrics(), uv_a, uv_b, uv_c);
    const auto pos_a = prim.positions.LoadAt(geom_buffers, tri[0]);
    float4 color = float4(1.f, 1.f, 1.f, 1.f);
    if (mat.basecolor_tex != ~0)
        color *= mat_textures[mat.basecolor_tex].Sample(tex_sampler, uv);
    return float4(tri[0] / 1000.f, tri[1] / 1000.f, tri[2] / 1000.f, 1.f); //color;// float4(uv, 0.f, 1.f);
}

// Main ray tracing function with debug info
float4 trace_scene(uint2 pixel_coord, uint2 screen_size) 
{
    // Generate camera ray for this pixel
    Ray primary_ray = generate_camera_ray(pixel_coord, screen_size);
    
    // Debug: Test if we can create ray query (this will fail if TLAS binding is broken)
    RayQuery<RayQueryFlags::AcceptFirstAndEndSearch> query;
    
    // Normal ray tracing
    query.TraceRayInline(scene_tlas, 0xff, primary_ray);
    query.Proceed();
    
    // Check hit status
    if (query.CommittedStatus() == HitType::HitTriangle) {
        // Hit sphere, perform shading
        return shade_hit(query);
    } else {
        // Miss, return square checkerboard
        float2 uv = float2(pixel_coord) / float2(screen_size);
        float checker_size = 24.0f;
        float aspect_ratio = float(screen_size.x) / float(screen_size.y);
        float2 checker_uv = float2(uv.x * aspect_ratio, uv.y) * checker_size;
        uint2 checker_coord = uint2(checker_uv);
        bool is_even = ((checker_coord.x + checker_coord.y) % 2) == 0;
        float3 bg_color = is_even ? float3(0.18f, 0.18f, 0.18f) : float3(0.15f, 0.15f, 0.15f);
        return float4(bg_color, 1.0f);
    }
}

// Compute shader entry point
[[compute_shader("cs_main")]]
[[kernel_2d(16, 16)]]
void compute_main([[sv_thread_id]] uint3 thread_id) 
{
    uint2 screen_size = uint2(camera_constants.screenSize);
    uint2 pixel_coord = thread_id.xy;
    
    // Boundary check
    if (any(pixel_coord >= screen_size)) {
        return;
    }
    
    // Execute ray tracing and write directly to output texture
    float4 pixel_color = trace_scene(pixel_coord, screen_size);
    output_texture.Store(pixel_coord, pixel_color);
}