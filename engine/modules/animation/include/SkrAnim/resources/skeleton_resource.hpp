#pragma once
#include "SkrAnim/ozz/skeleton.h"
#include "SkrBase/types.h"
#include "SkrBase/types.h"
#include "SkrRT/resource/resource_factory.h"
#ifndef __meta__
    #include "SkrAnim/resources/skeleton_resource.generated.h" // IWYU pragma: export
#endif

namespace skr
{
sreflect_struct(guid = "1876BF35-E4DC-450B-B9D4-09259397F4BA")
SkeletonResource
{
    ozz::animation::Skeleton skeleton;
};

template <>
struct SKR_ANIM_API BinSerde<skr::SkeletonResource>
{
    static bool read(SBinaryReader* r, skr::SkeletonResource& v);
    static bool write(SBinaryWriter* w, const skr::SkeletonResource& v);
};

struct SKR_ANIM_API SkelFactory : public ResourceFactory
{
public:
    virtual ~SkelFactory() noexcept = default;
    skr_guid_t GetResourceType() override;
    bool AsyncIO() override { return true; }
};
} // namespace skr