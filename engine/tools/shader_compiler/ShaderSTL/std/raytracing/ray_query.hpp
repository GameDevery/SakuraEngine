#pragma once
#include "../attributes.hpp"
#include "../types/ray.hpp"

namespace skr::shader {
enum struct RayQueryFlags : uint32
{
    None = 0x0,
    ForceOpaque = 0x1,
    ForceNonOpaque = 0x2,
    AcceptFirstAndEndSearch = 0x4,
    
    CullBackFace = 0x10,
    CullFrontFace = 0x20,

    CullOpaque = 0x40,
    CullNonOpaque = 0x80,

    CullTriangle = 0x100,
    CullProcedural = 0x200
};
inline constexpr RayQueryFlags operator|(RayQueryFlags lhs, RayQueryFlags rhs) {
    return static_cast<RayQueryFlags>(static_cast<uint32>(lhs) | static_cast<uint32>(rhs));
}

template <RayQueryFlags flags>
struct [[builtin("ray_query")]] RayQuery {
    RayQuery() = default;
    RayQuery(const RayQuery&) = delete;
    RayQuery(RayQuery&) = delete;
    RayQuery operator =(const RayQuery&) = delete;
    RayQuery operator =(RayQuery&) = delete;

    [[callop("RAY_QUERY_PROCEED")]] 
    bool proceed();
    
    [[callop("RAY_QUERY_COMMITTED_STATUS")]] 
    HitType committed_status();
    
    [[callop("RAY_QUERY_COMMITTED_TRIANGLE_BARYCENTRICS")]] 
    float2 committed_triangle_barycentrics();
    
    [[callop("RAY_QUERY_COMMITTED_INSTANCE_ID")]] 
    uint committed_instance_id();
    
    [[callop("RAY_QUERY_COMMITTED_PROCEDURAL_DISTANCE")]] 
    float committed_procedural_distance();
    
    [[callop("RAY_QUERY_COMMITTED_RAY_T")]] 
    float committed_ray_t();

    [[callop("RAY_QUERY_WORLD_RAY_ORIGIN")]] 
    float3 world_ray_origin();
    
    [[callop("RAY_QUERY_WORLD_RAY_DIRECTION")]] 
    float3 world_ray_direction();

    [[callop("RAY_QUERY_TRACE_RAY_INLINE")]] 
    void trace_ray_inline(Accel& AS, uint32 mask, const Ray& ray);

    // [[callop("RAY_QUERY_IS_TRIANGLE_CANDIDATE")]] bool is_triangle_candidate();
    // [[callop("RAY_QUERY_IS_PROCEDURAL_CANDIDATE")]] bool is_procedural_candidate();
    // [[callop("RAY_QUERY_WORLD_SPACE_RAY")]] Ray world_ray();
    // [[callop("RAY_QUERY_PROCEDURAL_CANDIDATE_HIT")]] ProceduralHit procedural_candidate();
    // [[callop("RAY_QUERY_TRIANGLE_CANDIDATE_HIT")]] TriangleHit triangle_candidate();
    // [[callop("RAY_QUERY_COMMITTED_HIT")]] CommittedHit committed_hit();
    // [[callop("RAY_QUERY_COMMIT_TRIANGLE")]] void commit_triangle();
    // [[callop("RAY_QUERY_COMMIT_PROCEDURAL")]] void commit_procedural(float distance);
    // [[callop("RAY_QUERY_TERMINATE")]] void terminate();
};

using RayQueryAll = RayQuery<RayQueryFlags::None>;
using RayQueryAny = RayQuery<RayQueryFlags::AcceptFirstAndEndSearch>;

}// namespace skr::shader