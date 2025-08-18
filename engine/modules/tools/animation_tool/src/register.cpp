#include "SkrAnim/resources/animation_resource.hpp"
#include "SkrAnim/resources/skin_resource.hpp"

#include "SkrAnimTool/animation_asset.h"
#include "SkrAnimTool/skeleton_asset.h"
#include "SkrAnimTool/skin_asset.h"

struct _AnimationToolRegister
{
    _AnimationToolRegister()
    {
#define _DEFAULT_COOKER(__COOKER_TYPE, __RESOURCE_TYPE) skd::asset::RegisterCooker<__COOKER_TYPE>(true, skr::RTTRTraits<__COOKER_TYPE>::get_guid(), skr::RTTRTraits<__RESOURCE_TYPE>::get_guid());
        _DEFAULT_COOKER(skd::asset::AnimCooker, skr::anim::AnimResource)
        _DEFAULT_COOKER(skd::asset::SkelCooker, skr::anim::SkeletonResource)
        _DEFAULT_COOKER(skd::asset::SkinCooker, skr::anim::SkinResource)
#undef _DEFAULT_COOKER

#define _IMPORTER(__TYPE) skd::asset::RegisterImporter<__TYPE>(skr::RTTRTraits<__TYPE>::get_guid());
        _IMPORTER(skd::asset::GltfAnimImporter)
        _IMPORTER(skd::asset::GltfSkelImporter)
#undef _IMPORTER
    }
} _animation_tool_register;