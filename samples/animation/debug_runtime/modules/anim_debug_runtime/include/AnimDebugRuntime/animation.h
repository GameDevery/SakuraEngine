#pragma once
/**
 * @file animation.h
 * @brief The Animation Module
 * @author sailing-innocent
 * @date 2025-06-30
 */
#include "SkrBase/config.h"
#include "SkrAnim/ozz/sampling_job.h"
#include "SkrRT/ecs/sugoi_meta.hpp"
#include "SkrAnim/resources/skeleton_resource.hpp"
#include "SkrAnim/ozz/base/maths/soa_transform.h"

#ifndef __meta__
    #include "AnimDebugRuntime/animation.generated.h" // IWYU pragma: export
#endif

namespace skr::anim
{
struct AnimResource;
struct AnimComponent;
} // namespace skr::anim

namespace animdbg
{

sreflect_managed_component(guid = "0197bff0-af37-761a-b489-b88840ee6411")
    anim_state_t {
    SKR_RESOURCE_FIELD(skr::anim::AnimResource, animation_resource);
    skr::Vector<ozz::math::SoaTransform> local_transforms;
    ozz::animation::SamplingJob::Context sampling_context;
    float                                current_time = 0.0f;
};

ANIM_DEBUG_RUNTIME_API void
InitializeANimState(anim_state_t* state, skr::anim::SkeletonResource* skeleton);

} // namespace animdbg
