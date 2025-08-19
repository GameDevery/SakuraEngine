#pragma once
#include "SkrAnim/resources/skeleton_resource.hpp"
#include "SkrRT/sugoi/sugoi_meta.hpp"
#ifndef __meta__
    #include "SkrAnim/components/skeleton_component.generated.h" // IWYU pragma: export
#endif

namespace skr
{

sreflect_managed_component(guid = "05622CB2-9D73-402B-B6C5-8075E13D5063")
SkeletonComponent
{
    skr::AsyncResource<skr::SkeletonResource> skeleton_resource;
};

} // namespace skr