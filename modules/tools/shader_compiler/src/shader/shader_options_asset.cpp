#include "SkrRenderer/resources/shader_meta_resource.hpp"
#include "SkrToolCore/asset/cook_system.hpp"
#include "SkrToolCore/project/project.hpp"
#include "SkrShaderCompiler/assets/shader_asset.hpp"
#include "SkrShaderCompiler/shader_compiler.hpp"
#include "SkrSerde/json_serde.hpp"

#include "SkrProfile/profile.h"

namespace skd::asset
{
void* ShaderOptionImporter::Import(skr::io::IRAMService* ioService, CookContext* context)
{
    skr::BlobId ioBuffer = {};
    context->AddSourceFileAndLoad(ioService, jsonPath.c_str(), ioBuffer);
    SKR_DEFER({ ioBuffer.reset(); });
    /*
    const auto assetRecord = context->GetAssetRecord();
    {
        SKR_LOG_FMT_ERROR(u8"Import shader options asset {} from {} failed, json parse error {}", assetRecord->guid, jsonPath, ::error_message(doc.error()));
        return nullptr;
    }
    '*/
    skr::String              jString(skr::StringView((const char8_t*)ioBuffer->get_data(), ioBuffer->get_size()));
    skr::archive::JsonReader jsonVal(jString.view());
    auto                     collection = SkrNew<skr_shader_options_resource_t>();
    skr::json_read(&jsonVal, *collection);
    return collection;
}

void ShaderOptionImporter::Destroy(void* resource)
{
    auto options = (skr_shader_options_resource_t*)resource;
    SkrDelete(options);
}

bool ShaderOptionsCooker::Cook(CookContext* ctx)
{
    const auto outputPath = ctx->GetOutputPath();
    //-----load config
    // no cook config for config, skipping

    //-----import resource object
    auto options = ctx->Import<skr_shader_options_resource_t>();
    if (!options) return false;
    SKR_DEFER({ ctx->Destroy(options); });

    //------write resource object to disk
    ctx->Save(*options);
    return true;
}

uint32_t ShaderOptionsCooker::Version()
{
    return kDevelopmentVersion;
}

} // namespace skd::asset