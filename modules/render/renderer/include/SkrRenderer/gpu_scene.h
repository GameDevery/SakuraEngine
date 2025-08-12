#pragma once
#include "SkrContainersDef/map.hpp"
#include "SkrGraphics/api.h"
#include "SkrRT/ecs/world.hpp"
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrRenderer/fwd_types.h"
#include "SkrRenderer/render_device.h"
#include "SkrRenderer/allocators/soa_segment.hpp"
#include "SkrRenderer/shared/soa_layout.hpp"
#include "SkrRenderer/shared/gpu_scene.hpp"
#ifndef __meta__
    #include "SkrRenderer/gpu_scene.generated.h" // IWYU pragma: export
#endif

namespace sugoi
{
struct archetype_t;
}
namespace skr::renderer
{

// 类型定义
using GPUSceneInstanceID = uint32_t;
using GPUArchetypeID = uint16_t;
using SOAIndex = uint32_t;
using CPUTypeID = skr::ecs::TypeIndex;
static constexpr GPUSceneInstanceID INVALID_GPU_SCENE_INSTANCE_ID = 0xFFFFFFFF;

// D3D12 24位 InstanceID 限制
struct GPUSceneCustomIndex
{
    GPUSceneCustomIndex() : packed(0x00FFFFFF) {}
    GPUSceneCustomIndex(uint32_t index) : packed(index) { SKR_ASSERT(index <= 0x00FFFFFF); }
    uint32_t GetInstanceID() const { return packed & 0x00FFFFFF; }
private:
    uint32_t packed;
};

sreflect_managed_component(guid = "fd6cd47d-bb68-4d1c-bd26-ad3717f10ea7")
GPUSceneInstance
{
    skr::ecs::Entity entity;
    GPUSceneInstanceID instance_index;
    GPUSceneCustomIndex custom_index;
};

struct GPUSceneComponentType
{
    CPUTypeID cpu_type_guid;
    SOAIndex soa_index;
    uint16_t element_size;
    uint16_t element_align;
    const char8_t* name;
};

struct GPUSceneConfig
{
    struct ComponentInfo
    {
        CPUTypeID cpu_type;
        SOAIndex soa_index;
        uint32_t element_size;
        uint32_t element_align;
    };
    
    skr::ecs::World* world = nullptr;
    skr::RendererDevice* render_device = nullptr;
    skr::Vector<ComponentInfo> components;  // 保留组件映射信息
    float resize_growth_factor = 1.5f;
};

class SKR_RENDERER_API GPUSceneBuilder
{
public:
    GPUSceneBuilder& with_world(skr::ecs::World* w)
    {
        config_.world = w;
        return *this;
    }
    GPUSceneBuilder& with_device(skr::RendererDevice* d)
    {
        config_.render_device = d;
        // 重新构造 soa_builder_ 以设置正确的 device
        soa_builder_.~Builder();
        new (&soa_builder_) SOASegmentBuffer::Builder(d->get_cgpu_device());
        return *this;
    }
    template <typename Layout>
    GPUSceneBuilder& from_layout(uint32_t initial_instances = 256);
    
    GPUSceneConfig build_config() { return config_; }
    const SOASegmentBuffer::Builder& get_soa_builder() const { return soa_builder_; }
    
private:
    friend struct GPUScene;
    GPUSceneConfig config_;
    SOASegmentBuffer::Builder soa_builder_;
};

// 主管理器
struct SKR_RENDERER_API GPUScene final
{
public:
    void Initialize(const struct GPUSceneConfig& config, const SOASegmentBuffer::Builder& soa_builder);
    void Shutdown();

    void AddEntity(skr::ecs::Entity entity);
    void RemoveEntity(skr::ecs::Entity entity);

    void RequireUpload(skr::ecs::Entity entity, CPUTypeID component);
    void ExecuteUpload(skr::render_graph::RenderGraph* graph);

    const skr::Map<CPUTypeID, SOAIndex>& GetTypeRegistry() const;
    SOAIndex GetComponentSOAIndex(const CPUTypeID& type_guid) const;
    bool IsComponentTypeRegistered(const CPUTypeID& type_guid) const;
    const GPUSceneComponentType* GetComponentType(SOAIndex local_id) const;

    inline skr::render_graph::BufferHandle GetSceneBuffer() const { return scene_buffer; }
    inline uint32_t GetInstanceCount() const { return instance_count; }

protected:
    void InitializeComponentTypes(const GPUSceneConfig& config);
    void AdjustBuffer(skr::render_graph::RenderGraph* graph);
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
    skr::Map<CPUTypeID, SOAIndex> type_registry;
    skr::Vector<GPUSceneComponentType> component_types;

    // instance counters
    GPUSceneInstanceID instance_count = 0;
    std::atomic<GPUSceneInstanceID> latest_index = 0;
    skr::ConcurrentQueue<GPUSceneInstanceID> free_ids;

    // core data 的 graph handle
    skr::render_graph::BufferHandle scene_buffer; 
    // SOA 分配器（包含 buffer）
    SOASegmentBuffer core_data;

private:

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

// Builder 实现已移至 SOASegmentBuffer::Builder

template <typename Layout>
inline GPUSceneBuilder& GPUSceneBuilder::from_layout(uint32_t initial_instances)
{
    // 清空现有配置
    config_.components.clear();
    
    // 让 SOASegmentBuffer::Builder 处理布局
    soa_builder_.from_layout<Layout>(initial_instances);
    
    // 同时构建组件映射信息
    [this]<typename... Cs>(typename Layout::template TypePack<Cs...>*) {
        skr::Vector<CPUTypeID> types;
        auto collect = [&types]<typename T>() {
            if constexpr (is_bundle_v<T>)
                []<typename... Bs>(ComponentBundle<Bs...>, auto& v) {
                    (v.add(sugoi_id_of<Bs>::get()), ...);
                }(T{}, types);
            else
                types.add(sugoi_id_of<T>::get());
        };
        (collect.template operator()<Cs>(), ...);
        
        Layout::for_each_component([this, &types](uint32_t id, uint32_t size, uint32_t align, uint32_t idx) {
            config_.components.add({types[idx], id, size, align});
        });
    }(static_cast<typename Layout::Elements*>(nullptr));
    
    return *this;
}

} // namespace skr::renderer