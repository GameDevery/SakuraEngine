#include "SkrToolCore/cook_system/cook_system.hpp"
#include "SkrToolCore/project/project.hpp"
#include "dxt_utils.hpp"
#include "SkrRT/io/ram_io.hpp"
#include "SkrProfile/profile.h"

namespace skd::asset
{
struct RawTextureData
{
    RawTextureData(skr::ImageDecoderId decoder, skr::io::IRAMService* ioService, skr::BlobId blob)
        : decoder(decoder)
        , ioService(ioService)
        , blob(blob)
    {
        decoder->initialize(blob->get_data(), blob->get_size());
    }

    ~RawTextureData()
    {
        if (decoder)
        {
            blob.reset();
            decoder.reset();
        }
    }

    skr::ImageDecoderId decoder = nullptr;
    skr::io::IRAMService* ioService = nullptr;
    skr::BlobId blob = nullptr;
};

void* TextureImporter::Import(skr::io::IRAMService* ioService, CookContext* context)
{
    skr::BlobId blob = nullptr;
    {
        SkrZoneScopedN("LoadFileDependencies");
        context->AddSourceFileAndLoad(ioService, assetPath.c_str(), blob);
    }

    {
        SkrZoneScopedN("TryDecodeTexture");
        // try decode texture
        const auto uncompressed_data = blob->get_data();
        const auto uncompressed_size = blob->get_size();
        EImageCoderFormat format = skr_image_coder_detect_format((const uint8_t*)uncompressed_data, uncompressed_size);
        if (auto decoder = skr::IImageDecoder::Create(format))
        {
            return SkrNew<RawTextureData>(decoder, ioService, blob);
        }
        else
        {
            SKR_DEFER({ blob.reset(); });
        }
    }
    return nullptr;
}

void TextureImporter::Destroy(void* resource)
{
    SkrDelete((RawTextureData*)resource);
}

bool TextureCooker::Cook(CookContext* ctx)
{
    auto uncompressed = ctx->Import<RawTextureData>();
    SKR_DEFER({ ctx->Destroy(uncompressed); });

    // try decode texture & calculate compressed format
    const auto decoder = uncompressed->decoder;
    const auto format = decoder->get_color_format();
    ECGPUFormat compressed_format = CGPU_FORMAT_UNDEFINED;
    switch (format) // TODO: format shuffle
    {
    case IMAGE_CODER_COLOR_FORMAT_Gray:
    case IMAGE_CODER_COLOR_FORMAT_GrayF:
        compressed_format = CGPU_FORMAT_DXBC4_UNORM;
        break;
    case IMAGE_CODER_COLOR_FORMAT_RGBA:
    default:
        compressed_format = CGPU_FORMAT_DXBC3_UNORM;
        break;
    }
    // DXT
    skr::Vector<uint8_t> compressed_data;
    {
        SkrZoneScopedN("DXTCompress");
        compressed_data = Util_DXTCompressWithImageCoder(decoder, compressed_format);
    }
    // TODO: ASTC
    // write texture resource
    STextureResource resource;
    resource.format = compressed_format;
    resource.mips_count = 1;
    resource.data_size = compressed_data.size();
    resource.height = decoder->get_height();
    resource.width = decoder->get_width();
    resource.depth = 1;
    {
        SkrZoneScopedN("SaveToCtx");
        if (!ctx->Save(resource))
            return false;
    }
    // write compressed files
    {
        SkrZoneScopedN("SaveToDisk");

        auto record = ctx->GetAssetMetaFile();
        auto resource_vfs = record->project->GetResourceVFS();
        auto extension = Util_CompressedTypeString(compressed_format);
        auto filename = skr::format(u8"{}.{}", record->guid, extension);
        
        auto compressed_file = skr_vfs_fopen(resource_vfs, filename.u8_str(), 
                                           SKR_FM_WRITE_BINARY, SKR_FILE_CREATION_ALWAYS_NEW);
        if (!compressed_file)
        {
            SKR_LOG_ERROR(u8"[TextureCooker] Failed to open compressed texture file for writing: %s", filename.c_str());
            return false;
        }
        SKR_DEFER({ skr_vfs_fclose(compressed_file); });
        skr_vfs_fwrite(compressed_file, compressed_data.data(), 0, compressed_data.size());
    }
    return true;
}

uint32_t TextureCooker::Version()
{
    return kDevelopmentVersion;
}

} // namespace skd::asset