#include <std/std.hpp>
#include "SkrRenderer/shared/database.hpp"

using namespace skr::gpu;

struct HIT { uint32_t inst; uint32 geom; } hit;
ByteAddressBuffer buffer;

[[compute_shader("database_debug"), kernel_2d(16, 16)]]
void database_debug()
{
    Instance inst = Row<Instance>(hit.inst).Load(buffer);
    Primitive prim = inst.primitives.LoadAt(buffer, hit.geom);
    uint32_t mat_index = prim.mat_index;
    
}
