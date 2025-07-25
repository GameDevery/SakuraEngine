#pragma once
// A Simple Debug Renderer for Scene Samples
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrBase/math.h"

namespace skr::scene
{
struct CameraData
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

class SCENE_RENDERER_API Renderer
{

public:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;
    // destructor
    ~Renderer() = default;

    void create_api_objects();
    void create_resources();
    void create_debug_pipeline();
    void build_render_graph(skr::render_graph::RenderGraph* graph, skr::render_graph::TextureHandle back_buffer);
    void finalize();

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

    CGPURenderPipelineId _debug_pipeline;

private:
    // viewport data for rendering
    uint32_t _width = 1280;
    uint32_t _height = 720;
    // camera data for rendering
    CameraData* p_camera_data = nullptr;
};

} // namespace skr::scene