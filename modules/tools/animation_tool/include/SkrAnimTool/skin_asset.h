#pragma once
#include "SkrToolCore/cook_system/cooker.hpp"
#ifndef __meta__
    #include "SkrAnimTool/skin_asset.generated.h" // IWYU pragma: export
#endif

namespace skd::asset
{
sreflect_struct(guid = "B863C921-3451-4024-A525-474D140099DB")
SKR_ANIMTOOL_API SkinCooker final : public Cooker
{
    bool Cook(CookContext * ctx) override;
    uint32_t Version() override { return kDevelopmentVersion; }
};
} // namespace skd::asset