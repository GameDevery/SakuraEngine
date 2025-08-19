#pragma once
#include "SkrRT/resource/resource_factory.h"
#include "SkrBase/types.h"
#include "SkrAnim/ozz/animation.h"
#ifndef __meta__
    #include "SkrAnim/resources/animation_resource.generated.h" // IWYU pragma: export
#endif

namespace skr
{
sreflect_struct(guid = "5D6DC46B-8696-4DD8-ADE4-C27D07CEDCCD")
AnimResource {
    ozz::animation::Animation animation;
};

template <>
struct SKR_ANIM_API BinSerde<AnimResource> {
    static bool read(SBinaryReader* r, AnimResource& v);
    static bool write(SBinaryWriter* w, const AnimResource& v);
};

class SKR_ANIM_API AnimFactory : public ResourceFactory
{
public:
    virtual ~AnimFactory() noexcept = default;
    skr_guid_t GetResourceType() override;
    bool       AsyncIO() override { return true; }
};
} // namespace skr