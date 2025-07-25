#pragma once
#include "SkrRT/config.h"
#include <SkrContainers/hashmap.hpp>
#include <SkrContainers/stl_deque.hpp>
#include "SkrGraphics/api.h"
#include "SkrBase/config.h"

namespace skr
{
namespace render_graph
{
class BufferPool
{
public:
    struct AllocationMark {
        uint64_t frame_index;
        uint32_t tags;
    };
    struct PooledBuffer {
        SKR_FORCEINLINE PooledBuffer() = delete;
        SKR_FORCEINLINE PooledBuffer(CGPUBufferId buffer, ECGPUResourceState state, AllocationMark mark)
            : buffer(buffer)
            , state(state)
            , mark(mark)
        {
        }
        CGPUBufferId       buffer;
        ECGPUResourceState state;
        AllocationMark     mark;
    };
    struct Key {
        const CGPUDeviceId      device         = nullptr;
        CGPUResourceTypes       descriptors    = CGPU_RESOURCE_TYPE_NONE;
        ECGPUMemoryUsage        memory_usage   = CGPU_MEM_USAGE_UNKNOWN;
        ECGPUFormat             format         = CGPU_FORMAT_UNDEFINED;
        CGPUBufferCreationFlags flags          = 0;
        uint64_t                first_element  = 0;
        uint64_t                element_count   = 0;
        uint64_t                element_stride = 0;
        uint64_t                padding        = 0;
        uint64_t                padding1       = 0;
        operator size_t() const;
        struct hasher {
            inline size_t operator()(const Key& val) const { return (size_t)val; }
        };

        friend class BufferPool;

        Key(CGPUDeviceId device, const CGPUBufferDescriptor& desc);
    };
    friend class RenderGraphBackend;
    void                                          initialize(CGPUDeviceId device);
    void                                          finalize();
    std::pair<CGPUBufferId, ECGPUResourceState> allocate(const CGPUBufferDescriptor& desc, AllocationMark mark, uint64_t min_frame_index);
    void                                          deallocate(const CGPUBufferDescriptor& desc, CGPUBufferId buffer, ECGPUResourceState final_state, AllocationMark mark);

protected:
    CGPUDeviceId                                                     device;
    skr::FlatHashMap<Key, skr::stl_deque<PooledBuffer>, Key::hasher> buffers;
};
} // namespace render_graph
} // namespace skr