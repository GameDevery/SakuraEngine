#pragma once
#include "SkrToolCore/cook_system/importer.hpp"
#include "SkrToolCore/cook_system/cooker.hpp"
#include "SkrToolCore/cook_system/asset_meta.hpp"

#ifndef __meta__
    #include "SkrAnimTool/skeleton_asset.generated.h" // IWYU pragma: export
#endif

namespace ozz::animation::offline
{
struct RawSkeleton;
struct RawAnimation;
} // namespace ozz::animation::offline

namespace skd::asset
{

using RawSkeleton = ozz::animation::offline::RawSkeleton;

sreflect_struct(
    guid = "1719ab02-7a48-45db-b101-949155f92cad" serde = @json)
SKR_ANIMTOOL_API GltfSkelImporter : public skd::asset::Importer
{
    // bool skeleton;
    // bool marker;
    // bool camera;
    // bool light;
    // bool null;
    // bool any;
    skr::String assetPath;
    virtual ~GltfSkelImporter() = default;
    virtual void* Import(skr::io::IRAMService*, CookContext * context) override;
    virtual void Destroy(void*) override;
    static uint32_t Version() { return kDevelopmentVersion; }
};

sreflect_struct(guid = "0198a872-01db-74ca-9382-8f1df4026ca0" serde = @json)
SkeletonAsset : public skd::asset::AssetMetadata
{
    int placeholder; // for future use
};

sreflect_struct(guid = "E3581419-8B44-4EF9-89FA-552DA6FE982A")
SKR_ANIMTOOL_API SkelCooker final : public Cooker
{
    bool Cook(CookContext * ctx) override;
    uint32_t Version() override { return kDevelopmentVersion; }
};
} // namespace skd::asset