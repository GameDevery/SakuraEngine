#pragma once
#include "SkrBase/config.h"
#include "SkrGraphics/api.h"
#include "SkrBase/math.h"
#include "SkrRT/resource/config_resource.h"

#ifndef __meta__
    #include "AnimDebugRuntime/renderer.generated.h" // IWYU pragma: export
#endif

namespace animd
{
// Lighting Struct
sreflect_struct(
    guid = "0197c86c-31f8-73cf-be27-d2a660396184"
    serde = @bin|@json
)
    LightingPushConstants {
    int bFlipUVX;
    int bFlipUVY;
};
// Lighting Push Constants
sreflect_struct(
    guid = "0197c86e-eb5e-739b-90b4-eda04b70ba5c"
    serde = @bin|@json
)
    LightingCSPushConstants {
    skr_float2_t viewportSize;
    skr_float2_t viewportOrigin;
};
// a dummy renderer for animation debug
class ANIM_DEBUG_RUNTIME_API Renderer
{
public:
    static LightingPushConstants   lighting_data;
    static LightingCSPushConstants lighting_cs_data;

public:
    // no-copy ctor
    Renderer()                           = default;
    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;
    // move ctor
    Renderer(Renderer&&)            = default;
    Renderer& operator=(Renderer&&) = default;
    // destructor
    ~Renderer() = default;

public:
    void create_api_objects();
    void create_resources();
    void create_render_pipeline();
    void finalize(); // get ready for loop
    void destroy();

public:
    // getters
    CGPUDeviceId         get_device() const { return _device; }
    ECGPUBackend         get_backend() const { return _backend; }
    CGPUInstanceId       get_instance() const { return _instance; }
    CGPUAdapterId        get_adapter() const { return _adapter; }
    CGPUQueueId          get_gfx_queue() const { return _gfx_queue; }
    CGPUSamplerId        get_static_sampler() const { return _static_sampler; }
    CGPUBufferId         get_index_buffer() const { return _index_buffer; }
    CGPUBufferId         get_vertex_buffer() const { return _vertex_buffer; }
    CGPUBufferId         get_instance_buffer() const { return _instance_buffer; }
    CGPURenderPipelineId get_pipeline() const { return _pipeline; }
    CGPURootSignatureId  get_root_signature() const { return _root_sig; }
    CGPUSwapChainId      get_swapchain() const { return _swapchain; }
    bool                 is_lock_FPS() const { return _lock_FPS; }
    bool                 is_aware_DPI() const { return _aware_DPI; }

    // setters
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
    void set_pipeline(CGPURenderPipelineId gbuffer_pipeline) { _pipeline = gbuffer_pipeline; }
    void set_root_signature(CGPURootSignatureId root_sig) { _root_sig = root_sig; }
    void set_swapchain(CGPUSwapChainId swapchain) { _swapchain = swapchain; }

private:
    // API Instance
    ECGPUBackend         _backend = CGPU_BACKEND_D3D12;
    CGPUDeviceId         _device;
    CGPUInstanceId       _instance;
    CGPUAdapterId        _adapter;
    CGPUQueueId          _gfx_queue;
    CGPUSamplerId        _static_sampler;
    CGPUBufferId         _index_buffer;
    CGPUBufferId         _vertex_buffer;
    CGPUBufferId         _instance_buffer;
    CGPURenderPipelineId _pipeline;
    CGPURootSignatureId  _root_sig;
    CGPUSwapChainId      _swapchain;

private:
    // state
    bool _lock_FPS  = true;
    bool _aware_DPI = false;
};

} // namespace animd