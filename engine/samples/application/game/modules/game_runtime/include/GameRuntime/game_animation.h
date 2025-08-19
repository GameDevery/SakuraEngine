#pragma once
#include "SkrBase/config.h"
#include "SkrAnim/resources/animation_resource.hpp"
#include "SkrAnim/resources/skeleton_resource.hpp"
#include "SkrAnim/resources/skin_resource.hpp"
#include "SkrAnim/ozz/base/maths/soa_transform.h"
#include "SkrAnim/ozz/sampling_job.h"
#include "SkrRT/sugoi/sugoi_meta.hpp"
#ifndef __meta__
    #include "GameRuntime/game_animation.generated.h" // IWYU pragma: export
#endif

namespace skr
{
struct AnimResource;
struct AnimComponent;
} // namespace skr

namespace game
{
sreflect_managed_component(guid = "E06E11F7-6F3A-4BFF-93D8-37310EF0FB87")
anim_state_t {
    SKR_RESOURCE_FIELD(skr::AnimResource, animation_resource);
    skr::Vector<ozz::math::SoaTransform> local_transforms;
    ozz::animation::SamplingJob::Context sampling_context;
    float                                currtime = 0.f;
};

GAME_RUNTIME_API void InitializeAnimState(anim_state_t* state, skr::SkeletonResource* skeleton);
GAME_RUNTIME_API void UpdateAnimState(anim_state_t* state, skr::SkeletonResource* skeleton, float dt, skr::AnimComponent* output);
} // namespace game