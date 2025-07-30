#include "helper.hpp"
#include "SkrBase/misc/make_zeroed.hpp"

namespace utils
{
DummyScene::DummyScene() {}

void DummyScene::init(skr::RendererDevice* render_device)
{
    auto cgpu_device = render_device->get_cgpu_device();
    auto vertex_size = sizeof(g_Positions) + 2 * sizeof(g_UVs) + sizeof(g_Normals);
    // auto vertex_size = sizeof(g_Positions);
    auto vb_desc = make_zeroed<CGPUBufferDescriptor>();
    vb_desc.name = u8"scene-renderer-vertices";
    vb_desc.size = vertex_size;
    vb_desc.memory_usage = CGPU_MEM_USAGE_CPU_TO_GPU;
    vb_desc.descriptors = CGPU_RESOURCE_TYPE_VERTEX_BUFFER;
    vb_desc.flags = CGPU_BCF_PERSISTENT_MAP_BIT;
    vb_desc.prefer_on_device = true;
    vertex_buffer = cgpu_create_buffer(cgpu_device, &vb_desc);
    {
        memcpy(vertex_buffer->info->cpu_mapped_address, g_Positions, sizeof(g_Positions));
        memcpy((uint8_t*)vertex_buffer->info->cpu_mapped_address + sizeof(g_Positions), g_UVs, sizeof(g_UVs));
        memcpy((uint8_t*)vertex_buffer->info->cpu_mapped_address + sizeof(g_Positions) + sizeof(g_UVs), g_UVs, sizeof(g_UVs));
        memcpy((uint8_t*)vertex_buffer->info->cpu_mapped_address + sizeof(g_Positions) + 2 * sizeof(g_UVs), g_Normals, sizeof(g_Normals));
    }

    auto index_size = sizeof(g_Indices);
    auto ib_desc = make_zeroed<CGPUBufferDescriptor>();
    ib_desc.name = u8"scene-renderer-indices";
    ib_desc.size = index_size;
    ib_desc.memory_usage = CGPU_MEM_USAGE_CPU_TO_GPU;
    ib_desc.descriptors = CGPU_RESOURCE_TYPE_INDEX_BUFFER;
    ib_desc.flags = CGPU_BCF_PERSISTENT_MAP_BIT;
    ib_desc.prefer_on_device = true;
    index_buffer = cgpu_create_buffer(cgpu_device, &ib_desc);

    {
        memcpy(index_buffer->info->cpu_mapped_address, g_Indices, sizeof(g_Indices));
    }
    // construct triangle vbvs and ibv
    skr_vertex_buffer_view_t vbv = {};
    vbv.buffer = vertex_buffer;
    vbv.stride = sizeof(skr_float3_t);
    vbv.offset = 0;
    vbvs.push_back(vbv);

    vbv.buffer = vertex_buffer;
    vbv.stride = sizeof(skr_float2_t);
    vbv.offset = sizeof(g_Positions);
    vbvs.push_back(vbv);

    vbv.buffer = vertex_buffer;
    vbv.stride = sizeof(skr_float2_t);
    vbv.offset = sizeof(g_Positions) + sizeof(g_UVs);
    vbvs.push_back(vbv);

    vbv.buffer = vertex_buffer;
    vbv.stride = sizeof(skr_float3_t);
    vbv.offset = sizeof(g_Positions) + 2 * sizeof(g_UVs);
    vbvs.push_back(vbv);

    SKR_LOG_INFO(u8"vbvs size: %d", vbvs.size());

    ibv.buffer = index_buffer;
    ibv.stride = sizeof(uint32_t);
    ibv.offset = 0;
    ibv.index_count = 3;
    ibv.first_index = 0;
}

skr::span<skr::renderer::PrimitiveCommand> DummyScene::get_primitive_commands()
{
    if (primitive_commands.is_empty())
    {
        auto& cmd = primitive_commands.emplace().ref();
        cmd.vbvs = { vbvs.data(), (uint32_t)vbvs.size() };
        cmd.ibv = &ibv;
    }

    return skr::span<skr::renderer::PrimitiveCommand>(primitive_commands.data(), (uint32_t)primitive_commands.size());
}

} // namespace utils