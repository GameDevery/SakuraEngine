#include <string.h>
#include "SkrGraphics/api.h"
#ifdef _WIN32
#include "SkrGraphics/extensions/cgpu_d3d12_exts.h"
#endif
#include "SkrCore/log.h"
#include "SkrBase/misc/make_zeroed.hpp"
#include "SkrCore/module/module_manager.hpp"

#include "SkrRenderer/skr_renderer.h"
#include "SkrRenderer/render_mesh.h"

IMPLEMENT_DYNAMIC_MODULE(SkrRendererModule, SkrRenderer);

namespace 
{
using namespace skr::literals;
const auto kGLTFVertexLayoutWithoutTangentId = u8"1b357a40-83ff-471c-8903-23e99d95b273"_guid;
const auto kGLTFVertexLayoutWithTangentId = u8"1b11e007-7cc2-4941-bc91-82d992c4b489"_guid;
const auto kGLTFVertexLayoutWithJointId = u8"C35BD99A-B0A8-4602-AFCC-6BBEACC90321"_guid;
}

void SkrRendererModule::on_load(int argc, char8_t** argv)
{
    SKR_LOG_TRACE(u8"skr renderer loaded!");
#ifdef _WIN32
    cgpu_d3d12_enable_DRED();
#endif
    // initailize render device
    auto builder = make_zeroed<skr::RenderDevice::Builder>();
    builder.enable_debug_layer = false;
    builder.enable_gpu_based_validation = false;
    builder.enable_set_name = true;
#if SKR_PLAT_WINDOWS
    builder.backend = CGPU_BACKEND_D3D12;
#elif SKR_PLAT_MACOSX
    builder.backend = CGPU_BACKEND_METAL;
#else
    builder.backend = CGPU_BACKEND_VULKAN;
#endif
    for (auto i = 0; i < argc; i++)
    {
        if (::strcmp((const char*)argv[i], "--vulkan") == 0)
        {
            builder.backend = CGPU_BACKEND_VULKAN;
        }
        else if (::strcmp((const char*)argv[i], "--d3d12") == 0)
        {
            builder.backend = CGPU_BACKEND_D3D12;
        }
        builder.enable_debug_layer |= (0 == ::strcmp((const char*)argv[i], "--debug_layer"));
        builder.enable_gpu_based_validation |= (0 == ::strcmp((const char*)argv[i], "--gpu_based_validation"));
        builder.enable_set_name |= (0 == ::strcmp((const char*)argv[i], "--gpu_obj_name"));
    }
    render_device = skr::RenderDevice::Create(builder);

    // register vertex layout
    {
        CGPUVertexLayout vertex_layout = {};
        vertex_layout.attributes[0] = { u8"POSITION", 1, CGPU_FORMAT_R32G32B32_SFLOAT, 0, 0, sizeof(skr_float3_t), CGPU_INPUT_RATE_VERTEX };
        vertex_layout.attributes[1] = { u8"TEXCOORD", 1, CGPU_FORMAT_R32G32_SFLOAT, 1, 0, sizeof(skr_float2_t), CGPU_INPUT_RATE_VERTEX };
        vertex_layout.attributes[2] = { u8"TEXCOORD", 1, CGPU_FORMAT_R32G32_SFLOAT, 2, 0, sizeof(skr_float2_t), CGPU_INPUT_RATE_VERTEX };
        vertex_layout.attributes[3] = { u8"NORMAL", 1, CGPU_FORMAT_R32G32B32_SFLOAT, 3, 0, sizeof(skr_float3_t), CGPU_INPUT_RATE_VERTEX };
        vertex_layout.attributes[4] = { u8"TANGENT", 1, CGPU_FORMAT_R32G32B32A32_SFLOAT, 4, 0, sizeof(skr_float4_t), CGPU_INPUT_RATE_VERTEX };
        vertex_layout.attributes[5] = { u8"JOINTS", 1, CGPU_FORMAT_R32G32B32A32_UINT, 5, 0, sizeof(uint32_t) * 4, CGPU_INPUT_RATE_VERTEX };
        vertex_layout.attributes[6] = { u8"WEIGHTS", 1, CGPU_FORMAT_R32G32B32A32_SFLOAT, 6, 0, sizeof(skr_float4_t), CGPU_INPUT_RATE_VERTEX };
        vertex_layout.attribute_count = 7;
        skr_mesh_resource_register_vertex_layout(::kGLTFVertexLayoutWithJointId, u8"SkinnedMesh", &vertex_layout);
        vertex_layout.attribute_count = 4;
        skr_mesh_resource_register_vertex_layout(::kGLTFVertexLayoutWithoutTangentId, u8"StaticMeshWithoutTangent", &vertex_layout);
        vertex_layout.attribute_count = 5;
        skr_mesh_resource_register_vertex_layout(::kGLTFVertexLayoutWithTangentId, u8"StaticMeshWithTangent", &vertex_layout);
    }
}

void SkrRendererModule::on_unload()
{
    SKR_LOG_TRACE(u8"skr renderer unloaded!");

    skr::RenderDevice::Destroy(render_device);
}

SkrRendererModule* SkrRendererModule::Get()
{
    auto mm = skr_get_module_manager();
    static auto rm = static_cast<SkrRendererModule*>(mm->get_module(u8"SkrRenderer"));
    return rm;
}

SRenderDeviceId SkrRendererModule::get_render_device()
{
    return render_device;
}

CGPUSamplerId skr_render_device_get_linear_sampler(SRenderDeviceId device)
{
    return device->get_linear_sampler();
}

CGPURootSignaturePoolId skr_render_device_get_root_signature_pool(SRenderDeviceId device)
{
    return device->get_root_signature_pool();
}

CGPUQueueId skr_render_device_get_gfx_queue(SRenderDeviceId device)
{
    return device->get_gfx_queue();
}

CGPUQueueId skr_render_device_get_cpy_queue(SRenderDeviceId device)
{
    return device->get_cpy_queue();
}

CGPUQueueId skr_render_device_get_nth_cpy_queue(SRenderDeviceId device, uint32_t n)
{
    return device->get_cpy_queue(n);
}

CGPUQueueId skr_render_device_get_compute_queue(SRenderDeviceId device)
{
    return device->get_compute_queue();
}

CGPUQueueId skr_render_device_get_nth_compute_queue(SRenderDeviceId device, uint32_t n)
{
    return device->get_compute_queue(n);
}

CGPUDeviceId skr_render_device_get_cgpu_device(SRenderDeviceId device)
{
    return device->get_cgpu_device();
}

CGPUDStorageQueueId skr_render_device_get_file_dstorage_queue(SRenderDeviceId device)
{
    return device->get_file_dstorage_queue();
}

CGPUDStorageQueueId skr_render_device_get_memory_dstorage_queue(SRenderDeviceId device)
{
    return device->get_memory_dstorage_queue();
}