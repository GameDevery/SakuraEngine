#pragma once
#include "SkrToolCore/fwd_types.hpp"
#ifndef __meta__
    #include "SkrToolCore/asset/cooker.generated.h"
#endif

namespace skd
{
namespace asset
{
sreflect_struct(guid = "ff344604-b522-411c-b9a5-1ec4b5970c02")
TOOL_CORE_API Cooker {
    static constexpr uint32_t kDevelopmentVersion = UINT32_MAX;
    virtual ~Cooker() {}
    virtual uint32_t Version()               = 0;
    virtual bool     Cook(CookContext* ctx) = 0;
    CookSystem*     system;
};

TOOL_CORE_API CookSystem* GetCookSystem();
TOOL_CORE_API void         RegisterCookerToSystem(CookSystem* system, bool isDefault, skr_guid_t cooker, skr_guid_t type, Cooker* instance);

template <class T>
void RegisterCooker(bool isDefault, skr_guid_t cookerGuid, skr_guid_t resGuid)
{
    static T instance;
    skd::asset::RegisterCookerToSystem(GetCookSystem(), isDefault, cookerGuid, resGuid, &instance);
}
} // namespace asset
} // namespace skd