#include "SkrToolCore/asset/cook_system.hpp"
#include "SkrToolCore/project/project.hpp"
#include "dxt_utils.hpp"
#include "SkrRT/io/ram_io.hpp"
#include "SkrCore/log.hpp"

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
    const auto outputPath = ctx->GetOutputPath();
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

        auto extension = Util_CompressedTypeString(compressed_format);
        auto compressed_path = outputPath;
        compressed_path.replace_extension(extension.c_str());
        auto compressed_pathstr = compressed_path.string();
        auto compressed_file = fopen(compressed_pathstr.c_str(), "wb");
        SKR_DEFER({ fclose(compressed_file); });
        fwrite(compressed_data.data(), compressed_data.size(), 1, compressed_file);
    }
    return true;
}

uint32_t TextureCooker::Version()
{
    return kDevelopmentVersion;
}

} // namespace skd::asset