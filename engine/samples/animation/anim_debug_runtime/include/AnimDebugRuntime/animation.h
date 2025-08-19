#pragma once
#include "SkrRT/sugoi/sugoi_meta.hpp"
#include "SkrAnim/ozz/sampling_job.h"
#include "SkrAnim/ozz/base/maths/soa_transform.h"
#include "SkrAnim/resources/animation_resource.hpp"
#include "SkrAnim/resources/skeleton_resource.hpp"

#ifndef __meta__
    #include "AnimDebugRuntime/animation.generated.h" // IWYU pragma: export
#endif

namespace skr
{
struct AnimResource;
struct AnimComponent;
} // namespace skr

namespace animdbg
{

sreflect_managed_component(guid = "0197bff0-af37-761a-b489-b88840ee6411")
anim_state_t
{
    skr::AsyncResource<skr::AnimResource> animation_resource;
    skr::Vector<ozz::math::SoaTransform> local_transforms;
    ozz::animation::SamplingJob::Context sampling_context;
    float current_time = 0.0f;
};

ANIM_DEBUG_RUNTIME_API void
InitializeAnimState(anim_state_t* state, skr::SkeletonResource* skeleton);

} // namespace animdbg
