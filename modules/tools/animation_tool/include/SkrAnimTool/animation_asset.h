#pragma once
#include "SkrRT/resource/resource_handle.h"
#include "SkrToolCore/asset/importer.hpp"
#include "SkrToolCore/asset/cooker.hpp"
#ifndef __meta__
    #include "SkrAnimTool/animation_asset.generated.h" // IWYU pragma: export
#endif

namespace skr::anim
{
struct SkeletonResource;
} // namespace skr::anim

namespace ozz::animation::offline
{
struct RawSkeleton;
struct RawAnimation;
} // namespace ozz::animation::offline

namespace skd::asset
{
using RawAnimation = ozz::animation::offline::RawAnimation;

sreflect_struct(guid = "37d07586-0901-480a-8dcd-1f1f8220569c" serde = @json)
SKR_ANIMTOOL_API GltfAnimImporter : public Importer
{
    skr::String assetPath;
    skr::String animationName;
    float samplingRate = 30.f;
    virtual ~GltfAnimImporter() = default;
    virtual void* Import(skr::io::IRAMService*, CookContext * context) override;
    virtual void Destroy(void*) override;
    static uint32_t Version() { return kDevelopmentVersion; }
};

sreflect_enum_class(guid = "544116F5-EBE9-4837-AB88-4743435F39EF" serde = @json)
SAnimAdditiveReference : uint32_t{
    animation,
    skeleton
};

sreflect_struct(guid = "9B780FFE-FA11-4BA9-B410-B5D5B2849E64" serde = @json)
SAnimOptimizationOverride
{
    /*
    {
        "name" : "*", //  Joint name. Wildcard characters '*' and '?' are supported
        "tolerance" : 0.001, //  The maximum error that an optimization is allowed to generate on a whole joint hierarchy.
        "distance" : 0.1 //  The distance (from the joint) at which error is measured. This allows to emulate effect on skinning.
    }
    */
    skr::String name = u8"*";
    float tolerance = 0.001f;
    float distance = 0.1f;
};

sreflect_struct(guid = "13873706-F7EE-4386-B7F0-B4E313864624" serde = @json)
AnimAssetMetadata
{
    /*
        "additive" : false, //  Creates a delta animation that can be used for additive blending.
        "additive_reference" : "animation", //  Select reference pose to use to build additive/delta animation. Can be "animation" to use the 1st animation keyframe as reference, or "skeleton" to use skeleton rest pose.
        "sampling_rate" : 0, //  Selects animation sampling rate in hertz. Set a value <= 0 to use imported scene default frame rate.
        "optimize" : true, //  Activates keyframes reduction optimization.
        "tolerance" : 0.001, //  The maximum error that an optimization is allowed to generate on a whole joint hierarchy.
        "distance" : 0.1, //  The distance (from the joint) at which error is measured. This allows to emulate effect on skinning.
        "override" :
        [
            {
            "name" : "*", //  Joint name. Wildcard characters '*' and '?' are supported
            "tolerance" : 0.001, //  The maximum error that an optimization is allowed to generate on a whole joint hierarchy.
            "distance" : 0.1 //  The distance (from the joint) at which error is measured. This allows to emulate effect on skinning.
            }
        ]
    */
    skr::resource::AsyncResource<skr::anim::SkeletonResource> skeletonAsset;
    bool additive = false;                                                        //  Creates a delta animation that can be used for additive blending.
    SAnimAdditiveReference additiveReference = SAnimAdditiveReference::animation; //  Select reference pose to use to build additive/delta animation. Can be "animation" to use the 1st animation keyframe as reference, or "skeleton" to use skeleton rest pose.
    float samplingRate = 0.f;                                                     //  Selects animation sampling rate in hertz. Set a value <= 0 to use imported scene default frame rate.
    bool optimize = true;                                                         //  Activates keyframes reduction optimization.
    float tolerance = 0.001f;                                                     //  The maximum error that an optimization is allowed to generate on a whole joint hierarchy.
    float distance = 0.1f;                                                        //  The distance (from the joint) at which error is measured. This allows to emulate effect on skinning.
    skr::Vector<SAnimOptimizationOverride> override;                              //  Per joint optimization setting override
};

sreflect_struct(guid = "81F1C813-1ABA-41BE-8D7A-F6C88E73E891")
SKR_ANIMTOOL_API AnimCooker : public skd::asset::Cooker
{
    bool Cook(CookContext * ctx) override;
    uint32_t Version() override { return kDevelopmentVersion; }
};
} // namespace skd::asset