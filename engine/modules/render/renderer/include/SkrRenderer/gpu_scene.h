#pragma once
#include "SkrContainersDef/map.hpp"
#include "SkrGraphics/api.h"
#include "SkrGraphics/raytracing.h"
#include "SkrRT/ecs/world.hpp"
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrRenderGraph/frame_resource.hpp"
#include "SkrRenderer/fwd_types.h"
#include "SkrRenderer/render_device.h"
#include "SkrRenderer/graphics/soa_segment.hpp"
#include "SkrRenderer/shared/soa_layout.hpp"
#ifndef __meta__
    #include "SkrRenderer/gpu_scene.generated.h" // IWYU pragma: export
#endif

namespace sugoi
{
struct archetype_t;
}
namespace skr
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
    GPUSceneInstanceID instance_index = ~0;
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

    // TODO: ADD A TRACKER COMPONENT TO REPLACE THIS KIND OF API
    bool CanRemoveEntity(skr::ecs::Entity entity);

    void AddEntity(skr::ecs::Entity entity);
    void RemoveEntity(skr::ecs::Entity entity);

    void RequireUpload(skr::ecs::Entity entity, CPUTypeID component);
    void ExecuteUpload(skr::render_graph::RenderGraph* graph);

    const skr::Map<CPUTypeID, SOAIndex>& GetTypeRegistry() const;
    SOAIndex GetComponentSOAIndex(const CPUTypeID& type_guid) const;
    bool IsComponentTypeRegistered(const CPUTypeID& type_guid) const;
    const GPUSceneComponentType* GetComponentType(SOAIndex local_id) const;

    inline skr::ecs::World* GetECSWorld() const { return ecs_world; }
    inline skr::render_graph::BufferHandle GetSceneBuffer() const { return scene_buffer; }
    skr::render_graph::AccelerationStructureHandle GetTLAS(skr::render_graph::RenderGraph* graph) const 
    { 
        // Return the current frame's TLAS handle
        return tlas_ctx.get(graph).tlas_handle;
    }
    inline uint32_t GetInstanceCount() const { return instance_count; }

protected:
    void InitializeComponentTypes(const GPUSceneConfig& config);
    void AdjustBuffer(skr::render_graph::RenderGraph* graph);

private:
    struct Upload
    {
        uint32_t src_offset; // 在 upload_buffer 中的偏移
        uint32_t dst_offset; // 在目标缓冲区中的偏移
        uint32_t data_size;  // 数据大小
    };
    void CreateSparseUploadPipeline(CGPUDeviceId device);
    void PrepareUploadBuffer(skr::render_graph::RenderGraph* graph);
    void DispatchSparseUpload(skr::render_graph::RenderGraph* graph, skr::Vector<Upload>&& core_uploads);

    friend struct GPUSceneInstanceTask;
    friend struct AddEntityToGPUScene;
    friend struct RemoveEntityFromGPUScene;
    friend struct ScanGPUScene;

    GPUSceneConfig config;
    skr::ecs::World* ecs_world = nullptr;
    skr::RendererDevice* render_device = nullptr;

    // TLAS
    skr::ConcurrentQueue<CGPUAccelerationStructureId> dirty_blases;
    skr::Vector<CGPUAccelerationStructureInstanceDesc> tlas_instances;
    
    // TLAS with FrameResource for automatic lifecycle management
    struct TLASContext
    {
        CGPUAccelerationStructureId tlas = nullptr;
        skr::render_graph::AccelerationStructureHandle tlas_handle;
        uint32_t instance_count = 0; // Track instance count for this TLAS
        
        // Single TLAS to discard when this frame comes around again
        CGPUAccelerationStructureId tlas_to_discard = nullptr;
        
        ~TLASContext()
        {
            // Cleanup any remaining TLAS when context is destroyed
            if (tlas_to_discard) cgpu_free_acceleration_structure(tlas_to_discard);
            if (tlas) cgpu_free_acceleration_structure(tlas);
        }
    };
    skr::render_graph::FrameResource<TLASContext> tlas_ctx;
    
    // Buffer with FrameResource for deferred destruction
    struct BufferContext
    {
        // Single buffer to discard when this frame comes around again
        CGPUBufferId buffer_to_discard = nullptr;
        
        ~BufferContext()
        {
            // Cleanup any remaining buffer when context is destroyed
            if (buffer_to_discard) cgpu_free_buffer(buffer_to_discard);
        }
    };
    skr::render_graph::FrameResource<BufferContext> buffer_ctx;

    // type registry
    skr::Map<CPUTypeID, SOAIndex> type_registry;
    skr::Vector<GPUSceneComponentType> component_types;

    // instance counters
    skr::ParallelFlatHashMap<skr::ecs::Entity, GPUSceneInstanceID, skr::Hash<skr::ecs::Entity>> entity_ids;
    std::atomic<GPUSceneInstanceID> instance_count = 0;
    std::atomic<GPUSceneInstanceID> latest_index = 0;
    skr::ConcurrentQueue<GPUSceneInstanceID> free_ids;
    std::atomic<GPUSceneInstanceID> free_id_count = 0;

    // core data 的 graph handle
    SOASegmentBuffer soa_segments;
    skr::render_graph::BufferHandle scene_buffer; 

private:
    static constexpr uint32_t kLaneCount = 2;
    struct UpdateLane
    {
        shared_atomic_mutex add_mtx;
        skr::Vector<skr::ecs::Entity> add_ents;

        shared_atomic_mutex remove_mtx;
        skr::Vector<skr::ecs::Entity> remove_ents;

        shared_atomic_mutex dirty_mtx;
        skr::Vector<skr::ecs::Entity> dirty_ents;

        std::atomic<uint64_t> dirty_comp_count = 0;
        
        skr::Map<skr::ecs::Entity, skr::InlineVector<CPUTypeID, 4>> dirties;
        uint64_t dirty_buffer_size;
    } lanes[kLaneCount];
    std::atomic_uint32_t front_lane = 0;

    UpdateLane& GetFrontLane() { return lanes[front_lane]; }
    UpdateLane& GetLaneForUpload() { return lanes[(front_lane + kLaneCount - 1) % kLaneCount]; }
    void SwitchLane() { front_lane = (front_lane + kLaneCount + 1) % kLaneCount; }
 
    // upload buffer management
    struct UploadContext
    {
        CGPUBufferId upload_buffer = nullptr;
        skr::Vector<Upload> soa_segments_uploads; 
        skr::Vector<uint8_t> DRAMCache;
        std::atomic<uint64_t> upload_cursor = 0; // upload_buffer 中的当前写入位置
    };
    skr::render_graph::FrameResource<UploadContext> upload_ctx;

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

} // namespace skr