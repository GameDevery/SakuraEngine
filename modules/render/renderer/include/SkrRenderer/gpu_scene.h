#pragma once
#include "SkrContainersDef/map.hpp"
#include "SkrGraphics/api.h"
#include "SkrRT/ecs/world.hpp"
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrRenderer/fwd_types.h"
#include "SkrRenderer/allocators/soa_segment.hpp"
#include "SkrRenderer/allocators/page_pool.hpp"
#ifndef __meta__
    #include "SkrRenderer/gpu_scene.generated.h" // IWYU pragma: export
#endif

namespace sugoi
{
struct archetype_t;
}
namespace skr::renderer
{
// 基础类型定义
struct GPUScenePageAllocator;
struct GPUSceneArchetype;
using GPUSceneInstanceID = uint32_t; // 32位足够，节省空间
using GPUArchetypeID = uint16_t;     // 最多 65536 个 archetype
using GPUPageID = uint32_t;          // 页面索引
using GPUComponentTypeID = uint16_t; // 组件类型 ID
using CPUTypeID = skr::ecs::TypeIndex;

// 特殊值定义
static constexpr GPUSceneInstanceID INVALID_GPU_SCENE_INSTANCE_ID = 0xFFFFFFFF;

// 实例 ID 编码（用于光线追踪等需要紧凑索引的场景）
// D3D12 限制 InstanceID 为 24 位，我们遵循这个限制
struct GPUSceneCustomIndex
{
    GPUSceneCustomIndex()
    {
        packed = 0x00FFFFFF;
    }

    GPUSceneCustomIndex(uint32_t index)
    {
        SKR_ASSERT(index <= 0x00FFFFFF); // 24位限制
        packed = index;
    }

    inline uint32_t GetInstanceID() const
    {
        return packed & 0x00FFFFFF;
    }

private:
    union
    {
        uint32_t packed;
        struct
        {
            uint32_t archetype_index : 10; // 1024 个 archetype
            uint32_t entity_index : 14;    // 16K 个实体/archetype
            uint32_t dontuse : 8;          // 保留，确保只使用 24 位
        };
    };
};

sreflect_managed_component(guid = "7d7ae068-1c62-46a0-a3bc-e6b141c8e56d")
GPUSceneObjectToWorld
{
    skr::float4x4 matrix;
};

sreflect_managed_component(guid = "d108318f-a1c2-4f64-b82f-63e7914773c8")
GPUSceneInstanceColor
{
    skr::float4 color;
};

sreflect_managed_component(guid = "e72c5ea1-9e31-4649-b190-45733b176760")
GPUSceneInstanceEmission
{
    skr::float4 color;
};

sreflect_managed_component(guid = "fd6cd47d-bb68-4d1c-bd26-ad3717f10ea7")
GPUSceneInstance
{
    skr::ecs::Entity entity;
    GPUSceneInstanceID instance_index;
    GPUSceneCustomIndex custom_index;
};

// 组件类型描述
struct GPUSceneComponentType
{
    CPUTypeID cpu_type_guid;        // CPU 端组件类型
    GPUComponentTypeID gpu_type_id; // GPU 端类型 ID
    uint16_t element_size;          // 组件大小（字节）
    uint16_t element_align;         // 对齐要求
    const char8_t* name;            // 调试用名称
};

// Archetype 的 GPU 映射
struct GPUSceneArchetype
{
    GPUSceneArchetype(sugoi::archetype_t* a, GPUArchetypeID i, skr::Vector<GPUComponentTypeID>&& types);

    // 基本标识
    const sugoi::archetype_t* cpu_archetype = nullptr;     // ECS archetype
    const GPUArchetypeID gpu_archetype_id = 0;             // GPU 端 ID
    const skr::Vector<GPUComponentTypeID> component_types; // 这个 Archetype 包含的组件类型
    std::atomic<GPUSceneInstanceID> latest_index = 0;      // 当前实体总数
    skr::ConcurrentQueue<GPUComponentTypeID> free_ids;     // free id
};

// 配置结构（分层设计）
struct GPUSceneConfig
{
    skr::ecs::World* world = nullptr;
    skr::RendererDevice* render_device = nullptr;
    size_t initial_size = 256 * 1024 * 1024;       // 256MB 核心数据
    size_t max_size = 1536 * 1024 * 1024;          // 1.5GB 最大核心数据
    bool enable_async_updates = true;                        // 异步更新
    skr::Map<CPUTypeID, GPUComponentTypeID> types; // 默认核心组件类型
    bool enable_auto_resize = true;                          // 是否开启自动扩容
    float auto_resize_threshold = 0.9f;                      // 自动扩容阈值
    float resize_growth_factor = 1.5f;                       // 扩容倍数
};

// 主管理器（分层设计）
struct SKR_RENDERER_API GPUScene final
{
public:
    void Initialize(CGPUDeviceId device, const struct GPUSceneConfig& config);
    void Shutdown();

    void AddEntity(skr::ecs::Entity entity);
    void RemoveEntity(skr::ecs::Entity entity);

    void RequireUpload(skr::ecs::Entity entity, CPUTypeID component);
    void ExecuteUpload(skr::render_graph::RenderGraph* graph);

    const skr::Map<CPUTypeID, GPUComponentTypeID>& GetTypeRegistry() const;
    GPUComponentTypeID GetComponentTypeID(const CPUTypeID& type_guid) const;
    bool IsComponentTypeRegistered(const CPUTypeID& type_guid) const;
    const GPUSceneComponentType* GetComponentType(GPUComponentTypeID type_id) const;

    inline skr::render_graph::BufferHandle GetSceneBuffer() const { return scene_buffer; }
    inline uint32_t GetInstanceCount() const { return instance_count; }

    uint32_t GetComponentSegmentOffset(GPUComponentTypeID type_id) const;
    uint32_t GetInstanceStride() const;

protected:
    void InitializeComponentTypes(const GPUSceneConfig& config);
    void CreateDataBuffer(CGPUDeviceId device, const GPUSceneConfig& config);
    void AdjustBuffer(skr::render_graph::RenderGraph* graph);
    uint32_t GetComponentTypeCount() const;
    uint64_t CalculateDirtySize() const;

private:
    struct Upload
    {
        uint32_t src_offset; // 在 upload_buffer 中的偏移
        uint32_t dst_offset; // 在目标缓冲区中的偏移
        uint32_t data_size;  // 数据大小
    };
    void CreateSparseUploadPipeline(CGPUDeviceId device);
    void PrepareUploadBuffer();
    void DispatchSparseUpload(skr::render_graph::RenderGraph* graph, skr::Vector<Upload>&& core_uploads, skr::Vector<Upload>&& additional_uploads);

    friend struct GPUSceneInstanceTask;
    friend struct AddEntityToGPUScene;
    friend struct RemoveEntityFromGPUScene;
    friend struct ScanGPUScene;

    GPUSceneConfig config;
    skr::ecs::World* ecs_world = nullptr;
    skr::RendererDevice* render_device = nullptr;

    // 类型注册表
    skr::Map<CPUTypeID, GPUComponentTypeID> type_registry;
    skr::Vector<GPUSceneComponentType> component_types;

    // Archetype 管理
    shared_atomic_mutex archetype_mutex;
    skr::Map<sugoi::archetype_t*, skr::SP<GPUSceneArchetype>> archetype_registry;

    // instance counters
    GPUSceneInstanceID instance_count = 0;
    std::atomic<GPUSceneInstanceID> latest_index = 0;
    skr::ConcurrentQueue<GPUSceneInstanceID> free_ids;

    skr::render_graph::BufferHandle scene_buffer; // core data 的 graph handle

    // 1. 核心数据：完全连续的大块（预分段）
    SOASegmentBuffer core_data; // SOA 分配器（包含 buffer）

    // dirties
    shared_atomic_mutex add_mtx;
    skr::Vector<skr::ecs::Entity> add_ents;

    shared_atomic_mutex remove_mtx;
    skr::Vector<skr::ecs::Entity> remove_ents;

    std::atomic<uint64_t> dirty_comp_count = 0;
    shared_atomic_mutex dirty_mtx;
    skr::Vector<skr::ecs::Entity> dirty_ents;
    skr::Map<skr::ecs::Entity, skr::InlineVector<CPUTypeID, 4>> dirties;

    // upload buffer management
    struct UploadContext
    {
        CGPUBufferId upload_buffer = nullptr;
        skr::Vector<Upload> core_data_uploads;       // Core Data上传操作
        skr::Vector<Upload> additional_data_uploads; // Additional Data上传操作
        skr::Vector<uint8_t> DRAMCache;
        std::atomic<uint64_t> upload_cursor = 0; // upload_buffer 中的当前写入位置
    } upload_ctx;

    // SparseUpload compute pipeline resources
    CGPUShaderLibraryId sparse_upload_shader = nullptr;
    CGPURootSignatureId sparse_upload_root_signature = nullptr;
    CGPUComputePipelineId sparse_upload_pipeline = nullptr;
};

inline GPUSceneArchetype::GPUSceneArchetype(sugoi::archetype_t* a, GPUArchetypeID i, skr::Vector<GPUComponentTypeID>&& types)
    : cpu_archetype(a)
    , gpu_archetype_id(i)
    , component_types(std::move(types))
{
}

} // namespace skr::renderer