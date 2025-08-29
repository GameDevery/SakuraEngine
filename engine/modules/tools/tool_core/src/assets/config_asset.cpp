#include "SkrBase/misc/defer.hpp"
#include "SkrRT/io/ram_io.hpp"
#include "SkrRT/resource/config_resource.h"
#include "SkrToolCore/assets/config_asset.hpp"
#include "SkrToolCore/cook_system/cook_system.hpp"
#include <SkrRTTR/type_registry.hpp>

namespace skd::asset
{
void* JsonConfigImporter::Import(skr::io::IRAMService* ioService, CookContext* context)
{
    const auto assetMetaFile = context->GetAssetMetaFile();
    auto type = skr::get_type_from_guid(configType);
    if (type == nullptr)
    {
        SKR_LOG_ERROR(u8"import resource %s failed, rtti is not load", assetMetaFile->GetURI().string().c_str());
        return nullptr;
    }

    skr::BlobId blob = nullptr;
    context->AddSourceFileAndLoad(ioService, assetPath.c_str(), blob);
    SKR_DEFER({ blob.reset(); });
    /*
    const auto assetMetaFile = context->GetAssetMetaFile();
    {
        SKR_LOG_FMT_ERROR(u8"Import shader options asset {} from {} failed, json parse error {}", assetMetaFile->guid, jsonPath, ::error_message(doc.error()));
        return nullptr;
    }
    '*/
    skr::String jString(skr::StringView((const char8_t*)blob->get_data(), blob->get_size()));
    skr::archive::JsonReader jsonVal(jString.view());

    auto resource = SkrNew<skr_config_resource_t>();
    resource->SetType(configType);
    SKR_UNIMPLEMENTED_FUNCTION();
    // type->read_json(resource->configData, doc.get_value().value_unsafe());
    // skr::json_read(&jsonVal, *resource);

    return resource;
}

void JsonConfigImporter::Destroy(void* resource)
{
    SkrDelete((skr_config_resource_t*)resource);
    return;
}

uint32_t ConfigCooker::Version()
{
    return 0;
}

bool ConfigCooker::Cook(CookContext* ctx)
{
    // const auto assetMetaFile = ctx->GetAssetMetaFile();
    //-----load config
    // no cook config for config, skipping
    //-----import resource object
    auto resource = ctx->Import<skr_config_resource_t>();
    if (!resource)
        return false;
    SKR_DEFER({ ctx->Destroy(resource); });
    //-----emit dependencies
    // no static dependencies
    //-----cook resource
    // no cook needed for config, just binarize it
    //-----fetch runtime dependencies
    // TODO. resume it
    // auto type = (skr::type::RecordType*)skr_get_type(&resource->configType);
    // for (auto& field : type->GetFields())
    // {
    //     if (field.type->Same(skr::type::type_of<SResourceHandle>::get()))
    //     {
    //         auto handle = (SResourceHandle*)((char*)resource->configData + field.offset);
    //         if (handle->is_null())
    //             continue;
    //         ctx->AddRuntimeDependency(handle->get_guid());
    //     }
    // }
    ctx->Save(*resource);
    return true;
}
} // namespace skd::asset