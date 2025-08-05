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

    // 分层标志：决定组件存储位置
    enum class StorageClass : uint8_t
    {
        CORE_DATA,      // 核心数据：预分段，连续存储，超高性能
        ADDITIONAL_DATA // 扩展数据：页面管理，支持不同 Archetype
    } storage_class;
};


// Archetype 的 GPU 映射
struct GPUSceneArchetype
{
    GPUSceneArchetype(sugoi::archetype_t* a, GPUArchetypeID i, skr::Vector<GPUComponentTypeID>&& types);

    // 基本标识
    const sugoi::archetype_t* cpu_archetype = nullptr;     // ECS archetype
    const GPUArchetypeID gpu_archetype_id = 0;             // GPU 端 ID
    const skr::Vector<GPUComponentTypeID> component_types; // 这个 Archetype 包含的组件类型
    std::atomic<GPUSceneInstanceID> latest_index = 0;               // 当前实体总数
    skr::ConcurrentQueue<GPUComponentTypeID> free_ids;     // free id
};

// GPU 端实例索引信息（用于快速查找）
struct alignas(16) GPUInstanceInfo
{
    uint16_t archetype_id;           // 所属 archetype
    uint16_t entity_index;           // 在 archetype 中的索引
    uint32_t core_data_offset;       // Core Data 基础偏移（可选优化）
};

// GPU 端组件地址索引（每个 instance 每个 additional component 一个条目）
struct alignas(16) GPUComponentAddressEntry
{
    uint32_t page_offset;            // 页面在 buffer 中的偏移
    uint16_t index_in_page;          // 在页面内的索引
    uint16_t element_size;           // 元素大小
};

// GPU 页表条目（用于页面管理）
struct GPUPageTableEntry
{
    uint32_t byte_offset;    // 在 HugeBuffer 中的字节偏移
    uint32_t byte_size;      // 页面大小（通常是 16KB）
    uint32_t element_count;  // 当前存储的元素数
    uint32_t element_stride; // 每个元素的字节大小
};

// 组件地址信息
struct ComponentAddress
{
    CGPUBufferId buffer;        // 组件数据所在的缓冲区
    uint32_t offset;           // 在缓冲区中的偏移量
    uint32_t stride;           // 组件大小（用于数组访问）
    bool is_core_data;         // 是否在 CoreData 中
    
    // 附加信息（用于 AdditionalData）
    PageID page_id = INVALID_PAGE_ID;           // 所在页面ID
    uint32_t index_in_page = 0;                 // 在页面内的索引
};

// 配置结构（分层设计）
struct GPUSceneConfig
{
    skr::ecs::World* world = nullptr;
    skr::RendererDevice* render_device = nullptr;

    // 核心数据配置（预分段）
    size_t core_data_initial_size = 256 * 1024 * 1024; // 256MB 核心数据
    size_t core_data_max_size = 1536 * 1024 * 1024;    // 1.5GB 最大核心数据

    // 扩展数据配置（页面管理）
    size_t additional_data_initial_size = 64 * 1024 * 1024; // 64MB 扩展数据池
    size_t additional_data_max_size = 512 * 1024 * 1024;    // 512MB 最大扩展数据
    uint32_t initial_page_count = 1024;                     // 初始页面数（16MB worth）

    // 其他配置
    bool enable_async_updates = true; // 异步更新

    // 默认核心组件（可配置哪些组件放入核心数据区）
    skr::Map<CPUTypeID, GPUComponentTypeID> core_data_types;       // 默认核心组件类型
    skr::Map<CPUTypeID, GPUComponentTypeID> additional_data_types; // 默认核心组件类型

    // 简单的 Resize 配置
    bool enable_auto_resize = true;     // 是否开启自动扩容
    float auto_resize_threshold = 0.9f; // 自动扩容阈值
    float resize_growth_factor = 1.5f;  // 扩容倍数
};

// 主管理器（分层设计）
struct SKR_RENDERER_API GPUScene final
{
public:
    // 主要接口
    void Initialize(CGPUDeviceId device, const struct GPUSceneConfig& config);
    void Shutdown();

    // 统一的实例和组件管理
    void AddEntity(skr::ecs::Entity entity);
    void RemoveEntity(skr::ecs::Entity entity);

    void RequireUpload(skr::ecs::Entity entity, CPUTypeID component);
    void ExecuteUpload(skr::render_graph::RenderGraph* graph);

    // 组件查询和管理
    const skr::Map<CPUTypeID, GPUComponentTypeID>& GetTypeRegistry() const;
    GPUComponentTypeID GetComponentTypeID(const CPUTypeID& type_guid) const;
    bool IsComponentTypeRegistered(const CPUTypeID& type_guid) const;
    const GPUSceneComponentType* GetComponentType(GPUComponentTypeID type_id) const;

    // 简化的内存信息
    struct MemoryUsageInfo
    {
        size_t core_data_used_bytes;
        size_t core_data_capacity_bytes;
        size_t additional_data_used_bytes;
        size_t additional_data_capacity_bytes;

        float core_usage_ratio() const { return core_data_capacity_bytes > 0 ? (float)core_data_used_bytes / core_data_capacity_bytes : 0.0f; }
        float additional_usage_ratio() const { return additional_data_capacity_bytes > 0 ? (float)additional_data_used_bytes / additional_data_capacity_bytes : 0.0f; }
    };
    MemoryUsageInfo GetMemoryUsageInfo() const;

    inline skr::render_graph::BufferHandle GetCoreDataBuffer() const { return core_data_buffer; }
    // inline skr::render_graph::BufferHandle GetAdditionalDataBuffer() const { return additional_data_buffer; }
    inline uint32_t GetInstanceCount() const { return instance_count; }

public: // Core Data
    uint32_t GetCoreComponentSegmentOffset(GPUComponentTypeID type_id) const;
    uint32_t GetCoreInstanceStride() const;

protected: // Core Data
    void InitializeComponentTypes(const GPUSceneConfig& config);
    void CreateCoreDataBuffer(CGPUDeviceId device, const GPUSceneConfig& config);
    void AdjustCoreBuffer(skr::render_graph::RenderGraph* graph);
    uint32_t GetCoreComponentTypeCount() const;
    uint64_t CalculateDirtySize() const;

protected: // Additional Data
    // void CreateAdditionalDataBuffers(CGPUDeviceId device, const GPUSceneConfig& config);
    // void InitializePageAllocator(CGPUDeviceId device, const GPUSceneConfig& config); // 更新：需要device参数
    // uint32_t GetAdditionalComponentTypeCount() const;
    // GPUPageID AllocateComponentPage(GPUArchetypeID archetype_id, GPUComponentTypeID component_type);
    // GPUSceneCustomIndex EncodeCustomIndex(GPUArchetypeID archetype_id, uint32_t entity_index_in_archetype);
    // void AdjustAdditionalBuffer(skr::render_graph::RenderGraph* graph);

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

    skr::render_graph::BufferHandle core_data_buffer; // core data 的 graph handle

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