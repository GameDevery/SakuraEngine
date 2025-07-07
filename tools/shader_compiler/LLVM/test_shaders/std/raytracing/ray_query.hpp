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
    CullOpaque = 0x8,
    CullNonOpaque = 0x10,
    CullFrontFace = 0x20,
    CullBackFace = 0x40,
    CullTriangle = 0x80,
    CullProcedural = 0x100
};
inline constexpr RayQueryFlags operator|(RayQueryFlags lhs, RayQueryFlags rhs) {
    return static_cast<RayQueryFlags>(static_cast<uint32>(lhs) | static_cast<uint32>(rhs));
}

template <RayQueryFlags flags>
struct [[builtin("ray_query")]] RayQuery {
    RayQuery(const RayQuery&) = delete;
    RayQuery(RayQuery&) = delete;
    RayQuery operator =(const RayQuery&) = delete;
    RayQuery operator =(RayQuery&) = delete;

    [[callop("RAY_QUERY_PROCEED")]] bool proceed();
    [[callop("RAY_QUERY_IS_TRIANGLE_CANDIDATE")]] bool is_triangle_candidate();
    [[callop("RAY_QUERY_IS_PROCEDURAL_CANDIDATE")]] bool is_procedural_candidate();
    [[callop("RAY_QUERY_WORLD_SPACE_RAY")]] Ray world_ray();
    [[callop("RAY_QUERY_PROCEDURAL_CANDIDATE_HIT")]] ProceduralHit procedural_candidate();
    [[callop("RAY_QUERY_TRIANGLE_CANDIDATE_HIT")]] TriangleHit triangle_candidate();
    [[callop("RAY_QUERY_COMMITTED_HIT")]] CommittedHit committed_hit();
    [[callop("RAY_QUERY_COMMIT_TRIANGLE")]] void commit_triangle();
    [[callop("RAY_QUERY_COMMIT_PROCEDURAL")]] void commit_procedural(float distance);
    [[callop("RAY_QUERY_TERMINATE")]] void terminate();
};

using RayQueryAll = RayQuery<RayQueryFlags::None>;
using RayQueryAny = RayQuery<RayQueryFlags::AcceptFirstAndEndSearch>;

}// namespace skr::shader