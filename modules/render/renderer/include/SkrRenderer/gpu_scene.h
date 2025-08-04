#pragma once
#include "SkrContainersDef/sparse_vector.hpp"
#include "SkrContainersDef/map.hpp"
#include "SkrGraphics/api.h"
#include "SkrRT/ecs/world.hpp"
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrRenderer/fwd_types.h"
#include "SkrRenderer/allocators/soa_segment.hpp"
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
static constexpr GPUPageID INVALID_GPU_PAGE_ID = 0xFFFFFFFF;

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
    GPUArchetypeID archetype_id;
    uint32_t entity_index_in_archetype; // archetype 内的索引
    GPUSceneInstanceID instance_index;
    GPUSceneCustomIndex custom_index;
    GPUSceneArchetype* pArchetype;
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

// 页面描述（16KB 对齐）
struct GPUScenePage
{
    static constexpr size_t PAGE_SIZE = 16 * 1024; // 16KB

    GPUPageID page_id;                 // 全局页面 ID
    GPUArchetypeID archetype_id;       // 所属 archetype
    GPUComponentTypeID component_type; // 存储的组件类型

    uint32_t entity_count;    // 当前存储的实体数
    uint32_t entity_capacity; // 最大容量
    uint32_t next_free_slot;  // 下一个空闲槽位

    uint64_t gpu_buffer_offset; // 在大缓冲池中的偏移
    uint32_t generation;        // 版本号，用于验证
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

// GPU 页表条目
struct GPUPageTableEntry
{
    uint32_t byte_offset;    // 在 HugeBuffer 中的字节偏移
    uint32_t byte_size;      // 页面大小（通常是 16KB）
    uint32_t element_count;  // 当前存储的元素数
    uint32_t element_stride; // 每个元素的字节大小
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

    skr::SP<GPUSceneArchetype> EnsureArchetype(sugoi::archetype_t* group);
    void RemoveArchetype(sugoi::archetype_t* group);

    // 组件查询和管理
    const skr::Map<CPUTypeID, GPUComponentTypeID>& GetTypeRegistry() const;
    GPUComponentTypeID GetComponentTypeID(const CPUTypeID& type_guid) const;
    bool IsComponentTypeRegistered(const CPUTypeID& type_guid) const;
    const GPUSceneComponentType* GetComponentType(GPUComponentTypeID type_id) const;

    // 获取组件在缓冲区中的地址信息
    struct ComponentAddress
    {
        CGPUBufferId buffer;
        uint32_t offset;
        uint32_t stride;
        bool is_core_data;
    };
    ComponentAddress GetComponentAddress(skr::ecs::Entity entity, const CPUTypeID& component_type) const;
    ComponentAddress GetComponentAddress(GPUSceneInstanceID instance_id, GPUComponentTypeID component_type) const;

    // GPU 访问辅助
    struct GPUAccessInfo
    {
        CGPUBufferId core_data_buffer;       // 核心数据缓冲区
        CGPUBufferId additional_data_buffer; // 扩展数据缓冲区
        CGPUBufferId page_table_buffer;      // 页表缓冲区
    };
    GPUAccessInfo GetGPUAccessInfo() const;

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

    inline skr::render_graph::BufferHandle GetSceneBuffer() const { return scene_buffer; }
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
    void CreateAdditionalDataBuffers(CGPUDeviceId device, const GPUSceneConfig& config);
    void InitializePageAllocator(const GPUSceneConfig& config);
    uint32_t GetAdditionalComponentTypeCount() const;
    GPUPageID AllocateComponentPage(GPUArchetypeID archetype_id, GPUComponentTypeID component_type);
    GPUSceneCustomIndex EncodeCustomIndex(GPUArchetypeID archetype_id, uint32_t entity_index_in_archetype);

private:
    void CreateSparseUploadPipeline(CGPUDeviceId device);
    void PrepareUploadBuffer();
    void DispatchSparseUpload(skr::render_graph::RenderGraph* graph);
    // TODO: REMOVE THIS: Helper functions for buffer creation
    CGPUBufferId CreateBuffer(CGPUDeviceId device, const char8_t* name, size_t size, ECGPUMemoryUsage usage = CGPU_MEM_USAGE_GPU_ONLY);

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

    // 1. 核心数据：完全连续的大块（预分段）
    SOASegmentBuffer core_data; // SOA 分配器（包含 buffer）
    skr::render_graph::BufferHandle scene_buffer; // core data 的 graph handle

    // 2. 扩展数据：页面管理（支持不同 Archetype）
    struct AdditionalDataRegion
    {
        CGPUBufferId buffer; // HugeBuffer
        size_t buffer_size;  // 总大小
        size_t bytes_used;   // 已使用字节数

        // 页表系统（与核心数据使用相同的页面分配算法）
        CGPUBufferId page_table_buffer;                // GPU 可见页表
        skr::Vector<GPUPageTableEntry> page_table_cpu; // CPU 页表镜像
    } additional_data;

    // 统一页面管理（两个区域共用）
    struct PageAllocator
    {
        // 简单的页面池
        skr::Vector<GPUPageID> free_pages; // 空闲页面列表
        uint32_t total_page_count;         // 总页面数
        uint32_t next_new_page_id;         // 下一个新页面ID

        // 简单分配
        GPUPageID AllocatePage()
        {
            if (!free_pages.is_empty())
            {
                GPUPageID id = free_pages.back();
                free_pages.pop_back();
                return id;
            }
            return next_new_page_id++;
        }

        void FreePage(GPUPageID page_id)
        {
            free_pages.push_back(page_id);
        }
    } page_allocator;
    skr::Vector<GPUScenePage> pages;

    shared_atomic_mutex add_mtx;
    skr::Vector<skr::ecs::Entity> add_ents;

    shared_atomic_mutex remove_mtx;
    skr::Vector<skr::ecs::Entity> remove_ents;

    GPUSceneInstanceID instance_count = 0;
    std::atomic<GPUSceneInstanceID> latest_index = 0;
    skr::ConcurrentQueue<GPUSceneInstanceID> free_ids;

    shared_atomic_mutex dirty_mtx;
    skr::Vector<skr::ecs::Entity> dirty_ents;
    skr::Map<skr::ecs::Entity, skr::InlineVector<CPUTypeID, 4>> dirties;

    // upload buffer management
    struct Upload
    {
        uint64_t src_offset; // 在 upload_buffer 中的偏移
        uint64_t dst_offset; // 在目标缓冲区中的偏移
        uint64_t data_size;  // 数据大小
    };
    CGPUBufferId upload_buffer = nullptr;
    skr::Vector<Upload> uploads;             // 记录拷贝操作的位置信息
    std::atomic<uint64_t> upload_cursor = 0; // upload_buffer 中的当前写入位置
    std::atomic<uint64_t> dirty_comp_count = 0;

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