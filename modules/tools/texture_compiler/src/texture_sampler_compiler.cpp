#include "SkrToolCore/cook_system/cook_system.hpp"
#include "SkrRT/io/ram_io.hpp"
#include "SkrToolCore/project/project.hpp"
#include "SkrRenderer/resources/texture_resource.h"
#include "SkrTextureCompiler/texture_sampler_asset.hpp"
#include "SkrSerde/json_serde.hpp"

namespace skd::asset
{

void* TextureSamplerImporter::Import(skr::io::IRAMService* ioService, CookContext* context)
{
    skr::BlobId blob = nullptr;
    context->AddSourceFileAndLoad(ioService, jsonPath.c_str(), blob);
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
    auto sampler_resource = SkrNew<STextureSamplerResource>();
    skr::json_read(&jsonVal, *sampler_resource);
    return sampler_resource;
}

void TextureSamplerImporter::Destroy(void* resource)
{
    auto sampler_resource = (STextureSamplerResource*)resource;
    SkrDelete(sampler_resource);
}

bool TextureSamplerCooker::Cook(CookContext* ctx)
{
    const auto outputPath = ctx->GetOutputPath();
    //-----load config
    // no cook config for config, skipping

    //-----import resource object
    auto sampler_resource = ctx->Import<STextureSamplerResource>();
    if (!sampler_resource) return false;
    SKR_DEFER({ ctx->Destroy(sampler_resource); });

    // write runtime resource to disk
    ctx->Save(*sampler_resource);
    return true;
}

} // namespace skd::asset