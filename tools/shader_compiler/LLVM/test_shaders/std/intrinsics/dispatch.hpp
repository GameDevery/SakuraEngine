#pragma once
#include "./../attributes.hpp"
#include "./../types/vec.hpp"

namespace skr::shader {

[[callop("AllMemoryBarrier")]] 
extern void AllMemoryBarrier();

[[callop("AllMemoryBarrierWithGroupSync")]] 
extern void AllMemoryBarrierWithGroupSync();

[[callop("DeviceMemoryBarrier")]] 
extern void DeviceMemoryBarrier();

[[callop("DeviceMemoryBarrierWithGroupSync")]] 
extern void DeviceMemoryBarrierWithGroupSync();

[[callop("GroupMemoryBarrier")]] 
extern void GroupMemoryBarrier();

[[callop("GroupMemoryBarrierWithGroupSync")]] 
extern void GroupMemoryBarrierWithGroupSync();

template <concepts::rw_buffer B, concepts::int_family T>
[[callop("InterlockedAdd")]]
extern T InterlockedAdd(B buffer, uint offset, T v);

// raster
[[callop("RASTER_DISCARD")]] 
extern void discard();

}