#pragma once
#include "SkrRT/io/vram_io.hpp"
#include "SkrBase/config.h"
#include "fwd_types.h"
#ifdef __cplusplus

namespace skr
{
struct SKR_RENDERER_API RendererDevice
{
    struct Builder
    {
        ECGPUBackend backend;
        bool enable_debug_layer = false;
        bool enable_gpu_based_validation = false;
        bool enable_set_name = true;
        uint32_t aux_thread_count = 0;
    };
    static RendererDevice* Create() SKR_NOEXCEPT;
    static void Free(RendererDevice* device) SKR_NOEXCEPT;
    
    virtual ~RendererDevice() = default;
    virtual void initialize(const Builder& builder) = 0;
    virtual void finalize() = 0;

    virtual void create_api_objects(const Builder& builder) = 0;

    virtual CGPUDeviceId get_cgpu_device() const = 0;
    virtual ECGPUBackend get_backend() const = 0;
    virtual CGPUQueueId get_gfx_queue() const = 0;
    virtual CGPUQueueId get_cpy_queue(uint32_t idx = 0) const = 0;
    virtual CGPUQueueId get_compute_queue(uint32_t idx = 0) const = 0;
    virtual CGPUDStorageQueueId get_file_dstorage_queue() const = 0;
    virtual CGPUDStorageQueueId get_memory_dstorage_queue() const = 0;
    virtual CGPUSamplerId get_linear_sampler() const = 0;
    virtual CGPURootSignaturePoolId get_root_signature_pool() const = 0;
};
} // namespace skr
#endif

SKR_EXTERN_C SKR_RENDERER_API 
ECGPUFormat skr_render_device_get_swapchain_format(SRenderDeviceId device);

SKR_EXTERN_C SKR_RENDERER_API 
CGPUSamplerId skr_render_device_get_linear_sampler(SRenderDeviceId device);

SKR_EXTERN_C SKR_RENDERER_API 
CGPURootSignaturePoolId skr_render_device_get_root_signature_pool(SRenderDeviceId device);

SKR_EXTERN_C SKR_RENDERER_API 
CGPUQueueId skr_render_device_get_gfx_queue(SRenderDeviceId device);

SKR_EXTERN_C SKR_RENDERER_API 
CGPUDStorageQueueId skr_render_device_get_file_dstorage_queue(SRenderDeviceId device);

SKR_EXTERN_C SKR_RENDERER_API 
CGPUDStorageQueueId skr_render_device_get_memory_dstorage_queue(SRenderDeviceId device);

SKR_EXTERN_C SKR_RENDERER_API 
CGPUQueueId skr_render_device_get_cpy_queue(SRenderDeviceId device);

SKR_EXTERN_C SKR_RENDERER_API 
CGPUQueueId skr_render_device_get_nth_cpy_queue(SRenderDeviceId device, uint32_t n);

SKR_EXTERN_C SKR_RENDERER_API 
CGPUQueueId skr_render_device_get_compute_queue(SRenderDeviceId device);

SKR_EXTERN_C SKR_RENDERER_API 
CGPUQueueId skr_render_device_get_nth_compute_queue(SRenderDeviceId device, uint32_t n);

SKR_EXTERN_C SKR_RENDERER_API 
CGPUDeviceId skr_render_device_get_cgpu_device(SRenderDeviceId device);