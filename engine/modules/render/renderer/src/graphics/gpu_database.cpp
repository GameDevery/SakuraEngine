#include "SkrRenderer/graphics/gpu_database.hpp"
#include <utility>

namespace skr::gpu
{

// ==================== Builder Implementation ====================

TableConfig::TableConfig(CGPUDeviceId device)
    : device_(device)
{
}

TableConfig& TableConfig::with_instances(uint32_t initial_instances)
{
    initial_instances = initial_instances;
    return *this;
}

TableConfig& TableConfig::with_page_size(uint32_t page_size)
{
    page_size = page_size;
    return *this;
}

TableConfig& TableConfig::add_component(TableSegmentID type_id, uint32_t element_size, uint32_t element_align)
{
    // 创建独立的Segment
    TableSegment segment;
    segment.stride = element_size;
    segment.align = element_align;
    segment.buffer_offset = 0; // 将在calculate_layout中计算
    segment.element_count = 0; // 将在initialize中设置

    uint32_t segment_index = segments_.size();
    segments_.add(segment);

    // 创建Component到Segment的映射
    TableComponentMapping mapping;
    mapping.component_id = type_id;
    mapping.segment_index = segment_index;
    mapping.element_offset = 0; // 独立Segment，偏移为0
    mapping.element_size = element_size;
    mapping.element_align = element_align;

    mappings_.add(mapping);

    return *this;
}

// ==================== TableInstance Implementation ====================

TableInstance::~TableInstance()
{
    release_buffer();

    capacity_bytes = 0;
    instance_capacity = 0;
    config.segments_.clear();
    config.mappings_.clear();
}

TableInstance::TableInstance(const TableConfig& cfg)
    : config(cfg)
{
    // 根据布局类型确定实际容量
    if (config.page_size == UINT32_MAX)
    {
        // 连续布局：直接使用请求的容量
        instance_capacity = config.initial_instances;
        SKR_LOG_INFO(u8"TableInstance: Continuous layout, capacity = %u instances", instance_capacity);
    }
    else
    {
        // 分页布局：容量必须是页大小的整数倍
        uint32_t page_count = (config.initial_instances + config.page_size - 1) / config.page_size;
        instance_capacity = page_count * config.page_size;
        SKR_LOG_INFO(u8"TableInstance: Paged layout, requested = %u, page_size = %u, page_count = %u, capacity = %u instances",
            config.initial_instances,
            config.page_size,
            page_count,
            instance_capacity);
    }

    // 设置所有Segment的element_count
    for (auto& segment : config.segments_)
    {
        segment.element_count = instance_capacity;
    }

    // 计算布局
    calculate_layout();

    // 创建 GPU 缓冲区
    create_buffer();
}

void TableInstance::Store(uint64_t offset, const void* data, uint64_t size)
{
    auto& ctx = updates[update_index];
    const uint64_t update_offset = ctx.bytes.size();
    ctx.bytes.append((const uint8_t*)data, size);

    uint64_t cursor = 0;
    const uint64_t stride = 4 * sizeof(float);
    while (cursor < size)
    {
        const auto slice_size = min(stride, size - cursor);
        const Update update = {
            .offset = update_offset + cursor,
            .size = stride
        };
        ctx.updates.add(update);
        cursor += 16;
    }
}

uint64_t TableInstance::bindless_index() const
{
    // TODO: SUPPORT BINDLESS
    return ~0;
}

void TableInstance::calculate_layout()
{
    uint32_t offset = 0;

    if (config.page_size == UINT32_MAX)
    {
        // 连续布局（传统 SOA）
        for (auto& segment : config.segments_)
        {
            // 对齐
            offset = (offset + segment.align - 1) & ~(segment.align - 1);

            segment.buffer_offset = offset;

            offset += segment.stride * instance_capacity;
        }

        capacity_bytes = offset;
    }
    else
    {
        // 分页布局
        // 基于 instance_capacity 计算需要的页数
        uint32_t page_count = (instance_capacity + config.page_size - 1) / config.page_size;

        // 计算每页的字节大小
        page_stride_bytes = 0;
        for (const auto& segment : config.segments_)
        {
            page_stride_bytes = (page_stride_bytes + segment.align - 1) & ~(segment.align - 1);
            page_stride_bytes += segment.stride * config.page_size;
        }
        page_stride_bytes = (page_stride_bytes + 255) & ~255; // 对齐 256 字节

        // 设置每个Segment在页内的偏移
        offset = 0;
        for (auto& segment : config.segments_)
        {
            offset = (offset + segment.align - 1) & ~(segment.align - 1);
            segment.buffer_offset = offset; // 页内偏移

            offset += segment.stride * config.page_size;
        }

        // 总容量字节数
        capacity_bytes = page_stride_bytes * page_count;
    }
}

uint32_t TableInstance::get_component_offset(TableSegmentID component_id, TableInstanceID instance_id) const
{
    // 查找组件映射
    for (const auto& mapping : config.mappings_)
    {
        if (mapping.component_id == component_id)
        {
            const auto& segment = config.segments_[mapping.segment_index];

            if (config.page_size == UINT32_MAX)
            {
                // 连续布局
                // 地址 = Segment基址 + 实例偏移 * Segment步长 + 组件在Segment内的偏移
                return segment.buffer_offset + (instance_id * segment.stride) + mapping.element_offset;
            }
            else
            {
                // 分页布局
                uint32_t page = instance_id / config.page_size;
                uint32_t offset_in_page = instance_id % config.page_size;
                uint32_t page_start = page * page_stride_bytes;

                // 地址 = 页起始 + Segment在页内基址 + 页内实例偏移 * Segment步长 + 组件在Segment内的偏移
                uint32_t result = page_start + segment.buffer_offset + (offset_in_page * segment.stride) + mapping.element_offset;

                return result;
            }
        }
    }
    SKR_LOG_WARN(u8"TableInstance: Component %u not found", component_id);
    return 0;
}

void TableInstance::create_buffer()
{
    if (!config.device_)
    {
        SKR_LOG_ERROR(u8"TableInstance: Device not set");
        return;
    }

    if (buffer)
    {
        SKR_LOG_WARN(u8"TableInstance: Buffer already created");
        return;
    }

    if (capacity_bytes == 0)
    {
        SKR_LOG_WARN(u8"TableInstance: No data to allocate");
        return;
    }

    CGPUBufferDescriptor buffer_desc = {};
    buffer_desc.name = u8"TableInstance";
    buffer_desc.size = capacity_bytes;
    buffer_desc.memory_usage = CGPU_MEM_USAGE_GPU_ONLY;
    buffer_desc.usages = CGPU_BUFFER_USAGE_SHADER_READ | CGPU_BUFFER_USAGE_SHADER_READWRITE;
    buffer_desc.flags = CGPU_BUFFER_FLAG_DEDICATED_BIT;

    buffer = cgpu_create_buffer(config.device_, &buffer_desc);
    if (!buffer)
        SKR_LOG_ERROR(u8"TableInstance: Failed to create GPU buffer");
    else
        SKR_LOG_INFO(u8"TableInstance: Created GPU buffer (%lld MB)", capacity_bytes / (1024 * 1024));
}

void TableInstance::release_buffer()
{
    if (buffer)
    {
        cgpu_free_buffer(buffer);
        buffer = nullptr;
        SKR_LOG_DEBUG(u8"TableInstance: Released GPU buffer");
    }
}

bool TableInstance::needs_resize(uint32_t required_instances) const
{
    return required_instances > instance_capacity;
}

CGPUBufferId TableInstance::resize(uint32_t new_instance_capacity)
{
    // 对于分页布局，确保容量是页大小的整数倍
    if (config.page_size != UINT32_MAX)
    {
        uint32_t page_count = (new_instance_capacity + config.page_size - 1) / config.page_size;
        new_instance_capacity = page_count * config.page_size;
    }

    if (new_instance_capacity <= instance_capacity)
    {
        SKR_LOG_WARN(u8"TableInstance: New capacity %u not greater than current %u",
            new_instance_capacity,
            instance_capacity);
        return nullptr;
    }

    // 保存旧 buffer
    CGPUBufferId old_buffer = buffer;

    // 新 buffer 需要重新开始状态跟踪
    state_tracker.reset();

    // 更新容量并重新计算布局
    uint32_t old_instance_capacity = instance_capacity;
    instance_capacity = new_instance_capacity;

    // 更新所有Segment的element_count
    for (auto& segment : config.segments_)
    {
        segment.element_count = instance_capacity;
    }

    // 重新计算布局
    calculate_layout();

    // 创建新 buffer
    buffer = create_buffer_with_capacity(new_instance_capacity);

    SKR_LOG_INFO(u8"TableInstance: Resized from %u to %u instances, new buffer size: %llu MB",
        old_instance_capacity,
        new_instance_capacity,
        capacity_bytes / (1024 * 1024));

    return old_buffer;
}

void TableInstance::copy_segments(skr::render_graph::RenderGraph* graph,
    skr::render_graph::BufferHandle src_buffer,
    skr::render_graph::BufferHandle dst_buffer,
    uint32_t instance_count) const
{
    if (instance_count == 0)
        return;

    // 添加拷贝 pass
    graph->add_copy_pass(
        [=, this](skr::render_graph::RenderGraph& g, skr::render_graph::CopyPassBuilder& builder) {
            builder.set_name(skr::format(u8"TableInstance_Copy-{}", (uint64_t)this).c_str());

            if (config.page_size == UINT32_MAX)
            {
                // 连续布局：按Segment拷贝
                for (const auto& segment : config.segments_)
                {
                    const uint32_t copy_size = segment.stride * instance_count;
                    const uint32_t offset = segment.buffer_offset;
                    builder.buffer_to_buffer(
                        src_buffer.range(offset, offset + copy_size),
                        dst_buffer.range(offset, offset + copy_size));
                }
            }
            else
            {
                // 分页布局：按页拷贝更高效
                uint32_t pages_to_copy = (instance_count + config.page_size - 1) / config.page_size;
                uint32_t last_page_instances = instance_count % config.page_size;
                if (last_page_instances == 0 && instance_count > 0)
                    last_page_instances = config.page_size;

                // 拷贝完整页
                for (uint32_t page = 0; page < pages_to_copy; page++)
                {
                    uint32_t page_offset = page * page_stride_bytes;

                    if (page < pages_to_copy - 1)
                    {
                        // 完整页：直接拷贝整页
                        builder.buffer_to_buffer(
                            src_buffer.range(page_offset, page_offset + page_stride_bytes),
                            dst_buffer.range(page_offset, page_offset + page_stride_bytes));
                    }
                    else
                    {
                        // 最后一页：可能需要部分拷贝
                        // 按Segment拷贝以避免越界
                        uint32_t offset_in_page = 0;
                        for (const auto& segment : config.segments_)
                        {
                            // 对齐
                            offset_in_page = (offset_in_page + segment.align - 1) & ~(segment.align - 1);
                            // 计算这个Segment在最后一页的大小
                            const uint32_t segment_size = segment.stride * last_page_instances;
                            // 拷贝
                            builder.buffer_to_buffer(
                                src_buffer.range(page_offset + offset_in_page,
                                    page_offset + offset_in_page + segment_size),
                                dst_buffer.range(page_offset + offset_in_page,
                                    page_offset + offset_in_page + segment_size));

                            // 下一个Segment的偏移
                            offset_in_page += segment.stride * config.page_size;
                        }
                    }
                }
            }
        },
        {});
}

CGPUBufferId TableInstance::create_buffer_with_capacity(uint32_t capacity)
{
    if (!config.device_)
    {
        SKR_LOG_ERROR(u8"TableInstance: Device not set");
        return nullptr;
    }

    // capacity_bytes 已经在 calculate_layout 中计算
    if (capacity_bytes == 0)
    {
        SKR_LOG_WARN(u8"TableInstance: No data to allocate");
        return nullptr;
    }

    CGPUBufferDescriptor buffer_desc = {};
    buffer_desc.name = u8"TableInstance";
    buffer_desc.size = capacity_bytes;
    buffer_desc.memory_usage = CGPU_MEM_USAGE_GPU_ONLY;
    buffer_desc.usages = CGPU_BUFFER_USAGE_SHADER_READ | CGPU_BUFFER_USAGE_SHADER_READWRITE;
    buffer_desc.flags = CGPU_BUFFER_FLAG_DEDICATED_BIT;

    CGPUBufferId new_buffer = cgpu_create_buffer(config.device_, &buffer_desc);
    if (!new_buffer)
    {
        SKR_LOG_ERROR(u8"TableInstance: Failed to create GPU buffer");
    }

    return new_buffer;
}

skr::render_graph::BufferHandle TableInstance::import_buffer(
    skr::render_graph::RenderGraph* graph,
    const char8_t* name)
{
    if (!buffer || !graph)
        return {};

    auto handle = graph->create_buffer(
        [=, this](skr::render_graph::RenderGraph& g, skr::render_graph::BufferBuilder& builder) {
            builder.set_name(name)
                .import(buffer, state_tracker.get_last_state())
                .allow_shader_readwrite();
        });
    graph->track_state(handle, state_tracker);

    return handle;
}

skr::RC<TableInstance> TableManager::CreateTable(const TableConfig& cfg)
{
    auto table = skr::RC<TableInstance>::New(cfg);
    mtx.lock();
    tables.add(table);
    mtx.unlock();
    return table;
}

void TableManager::UploadToGPU(skr::render_graph::RenderGraph& graph)
{
    auto& ctx = upload_ctxs.get(&graph);
    ctx.upload_size = 0;

    mtx.lock();
    tables.remove_all_if([](RCWeak<TableInstance> t) { return t.is_expired(); });
    mtx.unlock();

    // calculate size & upload infos
    mtx.lock_shared();
    for (auto table : tables)
    {
        auto tbl = table.lock();
        auto& tbl_ctx = tbl->GetUpdateContext();
        for (auto& update : tbl_ctx.updates)
            update.offset += ctx.upload_size;
        ctx.upload_size += tbl_ctx.bytes.size();
        ctx.ops_size += tbl_ctx.updates.size();
    }
    mtx.unlock_shared();

    const uint64_t ops_size = skr::max(1ull, ctx.ops_size) * sizeof(TableInstance::Update);
    auto ops_buffer = graph.create_buffer(
        [=](skr::render_graph::RenderGraph& g, skr::render_graph::BufferBuilder& builder) {
            builder.set_name(u8"GPUScene-upload_operations")
                .size(ops_size)
                .memory_usage(CGPU_MEM_USAGE_CPU_TO_GPU)
                .as_upload_buffer()
                .allow_shader_read()
                .prefer_on_device();
        });

    const uint64_t upload_size = ctx.upload_size;
    auto upload_buffer = graph.create_buffer(
        [=](skr::render_graph::RenderGraph& g, skr::render_graph::BufferBuilder& builder) {
            builder.set_name(u8"GPUScene-upload_bytes")
                .size(upload_size)
                .memory_usage(CGPU_MEM_USAGE_CPU_TO_GPU)
                .as_upload_buffer()
                .allow_shader_read()
                .prefer_on_device();
        });

    mtx.lock_shared();

    mtx.unlock_shared();
}

skr::RC<TableManager> TableManager::Create()
{
    return skr::RC<TableManager>::New();
}

} // namespace skr::gpu
