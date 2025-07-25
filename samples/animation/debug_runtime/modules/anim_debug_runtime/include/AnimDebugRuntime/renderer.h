#pragma once
#include "SkrBase/config.h"
#include "SkrGraphics/api.h"
#include "SkrBase/math.h"
#include "SkrRT/resource/config_resource.h"
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "AnimDebugRuntime/bone_geometry.h"
#include <SkrAnim/ozz/skeleton_utils.h>
#include <SkrAnim/ozz/local_to_model_job.h>

#ifndef __meta__
    #include "AnimDebugRuntime/renderer.generated.h" // IWYU pragma: export
#endif

namespace animd
{
// Lighting Struct
sreflect_struct(
    guid = "0197c86c-31f8-73cf-be27-d2a660396184" serde = @bin | @json)
LightingPushConstants
{
    int bFlipUVX;
    int bFlipUVY;
};
// Lighting Push Constants
sreflect_struct(
    guid = "0197c86e-eb5e-739b-90b4-eda04b70ba5c" serde = @bin | @json)
LightingCSPushConstants
{
    skr_float2_t viewportSize;
    skr_float2_t viewportOrigin;
};

sreflect_struct(
    guid = "0197ec89-5620-76b9-b08b-14602ca94b24" serde = @bin | @json)
Camera
{
    skr_float3_t position = {};                   // camera position
    skr_float3_t front = skr_float3_t::forward(); // camera front vector
    skr_float3_t up = skr_float3_t::up();         // camera up vector
    skr_float3_t right = skr_float3_t::right();   // camera right vector

    float fov = 3.1415926f / 2.f; // fov_x
    float aspect = 1.0;           // aspect ratio
    float near_plane = 0.1;       // near plane distance
    float far_plane = 1000.0;     // far plane distance
};

sreflect_struct(
    guid = "0197ec8b-f8dc-73ec-8635-e20dac02d900" serde = @bin | @json)
CameraControlState
{
    float move_speed = 1.0f;   // camera move speed
    float rotate_speed = 0.1f; // camera rotate speed
    float zoom_speed = 0.1f;   // camera zoom speed
};

// a dummy renderer for animation debug
class ANIM_DEBUG_RUNTIME_API Renderer
{
public:
    static LightingPushConstants lighting_data;
    static LightingCSPushConstants lighting_cs_data;

public:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;
    // destructor
    ~Renderer() = default;

public:
    void create_skeleton(ozz::animation::Skeleton& skeleton);
    void update_anim(ozz::animation::Skeleton& skeleton, ozz::span<ozz::math::Float4x4> prealloc_models);

    void create_api_objects();
    void create_resources();
    void create_gbuffer_pipeline();
    void create_lighting_pipeline();
    void create_blit_pipeline();
    void create_render_pipeline();
    void build_render_graph(skr::render_graph::RenderGraph* graph, skr::render_graph::TextureHandle back_buffer);
    void finalize();

public:
    // getters
    Camera* get_pcamera() { return mp_camera; }
    CGPUDeviceId get_device() const { return _device; }
    ECGPUBackend get_backend() const { return _backend; }
    CGPUInstanceId get_instance() const { return _instance; }
    CGPUAdapterId get_adapter() const { return _adapter; }
    CGPUQueueId get_gfx_queue() const { return _gfx_queue; }
    CGPUSamplerId get_static_sampler() const { return _static_sampler; }
    CGPUBufferId get_index_buffer() const { return _index_buffer; }
    CGPUBufferId get_vertex_buffer() const { return _vertex_buffer; }
    CGPUBufferId get_instance_buffer() const { return _instance_buffer; }
    bool is_lock_FPS() const { return _lock_FPS; }
    bool is_aware_DPI() const { return _aware_DPI; }

    // setters
    void set_pcamera(Camera* camera) { mp_camera = camera; }
    void set_lock_FPS(bool lock) { _lock_FPS = lock; }
    void set_aware_DPI(bool aware) { _aware_DPI = aware; }
    void set_device(CGPUDeviceId device) { _device = device; }
    void set_backend(ECGPUBackend backend) { _backend = backend; }
    void set_instance(CGPUInstanceId instance) { _instance = instance; }
    void set_adapter(CGPUAdapterId adapter) { _adapter = adapter; }
    void set_gfx_queue(CGPUQueueId gfx_queue) { _gfx_queue = gfx_queue; }
    void set_static_sampler(CGPUSamplerId static_sampler) { _static_sampler = static_sampler; }
    void set_index_buffer(CGPUBufferId index_buffer) { _index_buffer = index_buffer; }
    void set_vertex_buffer(CGPUBufferId vertex_buffer) { _vertex_buffer = vertex_buffer; }
    void set_instance_buffer(CGPUBufferId instance_buffer) { _instance_buffer = instance_buffer; }
    void set_width(uint32_t width) { _width = width; }
    void set_height(uint32_t height) { _height = height; }

private:
    ECGPUBackend _backend = CGPU_BACKEND_D3D12;
    CGPUDeviceId _device;
    CGPUInstanceId _instance;
    CGPUAdapterId _adapter;
    CGPUQueueId _gfx_queue;
    CGPUSamplerId _static_sampler;
    CGPUBufferId _index_buffer;
    CGPUBufferId _vertex_buffer;
    CGPUBufferId _instance_buffer;

    CGPURenderPipelineId _debug_pipeline; // debug draw pipeline
    CGPURenderPipelineId _gbuffer_pipeline;
    CGPURenderPipelineId _lighting_pipeline;
    CGPURenderPipelineId _blit_pipeline;

    const ECGPUFormat gbuffer_formats[2] = {
        CGPU_FORMAT_R8G8B8A8_UNORM, CGPU_FORMAT_R16G16B16A16_SNORM
    };
    const ECGPUFormat gbuffer_depth_format = CGPU_FORMAT_D32_SFLOAT;

private:
    // state
    bool _lock_FPS = true;
    bool _aware_DPI = false;
    uint32_t _width = 1280;
    uint32_t _height = 720;

    size_t _instance_count = 0;
    // skr_float4x4_t* _instance_data  = nullptr; // instance data for each bone
    skr::Vector<skr_float4x4_t> _instance_data; // instance data for each bone
    Camera* mp_camera = nullptr;                // camera for rendering
};

} // namespace animd