#pragma once
#include "SkrBase/types/guid.h"
#include "SkrToolCore/fwd_types.hpp"
#ifndef __meta__
    #include "SkrToolCore/asset/importer.generated.h" // IWYU pragma: export
#endif

namespace skd::asset
{
using namespace skr;
template <class T>
void RegisterImporter(skr::GUID guid);

sreflect_struct(guid = "76044661-E2C9-43A7-A4DE-AEDD8FB5C847"; serde = @json)
TOOL_CORE_API Importer
{
    static constexpr uint32_t kDevelopmentVersion = UINT32_MAX;

    virtual ~Importer() = default;
    virtual void* Import(skr::io::IRAMService*, CookContext* context) = 0;
    virtual void Destroy(void*) = 0;
    static uint32_t Version() { return kDevelopmentVersion; }
};

struct TOOL_CORE_API ImporterTypeInfo
{
    Importer* (*Load)(const AssetRecord* record, skr::archive::JsonReader* object);
    uint32_t (*Version)();
};

struct ImporterRegistry
{
    virtual Importer* LoadImporter(const AssetRecord* record, skr::archive::JsonReader* object, skr::GUID* pGuid = nullptr) = 0;
    virtual uint32_t GetImporterVersion(skr::GUID type) = 0;
    virtual void RegisterImporter(skr::GUID type, ImporterTypeInfo info) = 0;
};

TOOL_CORE_API ImporterRegistry* GetImporterRegistry();
} // namespace skd::asset

template <class T>
void skd::asset::RegisterImporter(skr::GUID guid)
{
    auto registry = GetImporterRegistry();
    auto loader =
        +[](const AssetRecord* record, skr::archive::JsonReader* object) -> Importer* {
        auto importer = SkrNew<T>();
        skr::json_read(object, *importer);
        return importer;
    };
    ImporterTypeInfo info{ loader, T::Version };
    registry->RegisterImporter(guid, info);
}