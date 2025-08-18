#pragma once
#include "SkrRT/io/ram_io.hpp"
#include "SkrRT/io/vram_io.hpp"
#include "SkrRTTR/enum_tools.hpp"
#include "SkrRenderer/fwd_types.h"
#ifndef __meta__
    #include "SkrRenderer/resources/texture_resource.generated.h" // IWYU pragma: export
#endif

// (GPU) texture resource
sreflect_struct(
    guid = "f8821efb-f027-4367-a244-9cc3efb3a3bf" serde = @bin)
STextureResource
{
    skr::EnumAsValue<ECGPUFormat> format;
    uint32_t mips_count;
    uint64_t data_size;
    uint32_t width;
    uint32_t height;
    uint32_t depth;

    sattr(serde = @disable)
    CGPUTextureId texture = nullptr;
    sattr(serde = @disable)
    CGPUTextureViewId texture_view = nullptr;
};
typedef struct STextureResource STextureResource;
typedef struct STextureResource* skr_texture_resource_id;

sreflect_enum_class(
    guid = "a9ff6d5f-620b-444b-8cb3-3b926ec1316e" serde = @bin | @json)
ESkrTextureSamplerFilterType SKR_IF_CPP( : uint32_t) {
    NEAREST,
    LINEAR
};

sreflect_enum_class(
    guid = "01eccfa6-ac6c-4195-b725-66c865529d6f" serde = @bin | @json)
ESkrTextureSamplerMipmapMode SKR_IF_CPP( : uint32_t) {
    NEAREST,
    LINEAR
};

sreflect_enum_class(
    guid = "b5dee0a2-5b20-4062-a036-79905b1d325f" serde = @bin | @json)
ESkrTextureSamplerAddressMode SKR_IF_CPP( : uint32_t) {
    MIRROR,
    REPEAT,
    CLAMP_TO_EDGE,
    CLAMP_TO_BORDER
};

sreflect_enum_class(
    guid = "566ef8d0-9c68-4578-be0b-b33781fc1a0f" serde = @bin | @json)
ESkrTextureSamplerCompareMode SKR_IF_CPP( : uint32_t) {
    NEVER,
    LESS,
    EQUAL,
    LEQUAL,
    GREATER,
    NOTEQUAL,
    GEQUAL,
    ALWAYS,
};

// (GPU) texture sampler resource
sreflect_struct(
    guid = "ab483a53-5024-48f2-87a7-9502063c97f3" serde = @bin | @json)
STextureSamplerResource
{
    ESkrTextureSamplerFilterType min_filter;
    ESkrTextureSamplerFilterType mag_filter;
    ESkrTextureSamplerMipmapMode mipmap_mode;
    ESkrTextureSamplerAddressMode address_u;
    ESkrTextureSamplerAddressMode address_v;
    ESkrTextureSamplerAddressMode address_w;
    float mip_lod_bias;
    float max_anisotropy;
    ESkrTextureSamplerCompareMode compare_func;

    sattr(serde = @disable)
    CGPUSamplerId sampler = nullptr;
};

#ifdef __cplusplus
#include "SkrRT/resource/resource_factory.h"

namespace skr::renderer
{
using SamplerFilterType = ESkrTextureSamplerFilterType;
using SamplerMipmapMode = ESkrTextureSamplerMipmapMode;
using SamplerAddressMode = ESkrTextureSamplerAddressMode;
using SamplerCompareMode = ESkrTextureSamplerCompareMode;
using SamplerResource = ::STextureSamplerResource;
using TextureResource = ::STextureResource;
} // namespace skr::renderer

namespace skr::resource
{
// - dstorage & bc: dstorage
// - dstorage & bc & zlib: dstorage with custom decompress queue
// - bc & zlib: [TODO] ram service & decompress service & upload
//    - upload with copy queue
//    - upload with gfx queue
struct SKR_RENDERER_API TextureFactory : public ResourceFactory
{
    virtual ~TextureFactory() = default;

    struct Root
    {
        skr_vfs_t* vfs = nullptr;
        const skr_char8* dstorage_root;
        skr::io::IRAMService* ram_service = nullptr;
        skr::io::IVRAMService* vram_service = nullptr;
        SRenderDeviceId render_device = nullptr;
    };

    float AsyncSerdeLoadFactor() override { return 2.5f; }
    [[nodiscard]] static TextureFactory* Create(const Root& root);
    static void Destroy(TextureFactory* factory);
};

struct SKR_RENDERER_API TextureSamplerFactory : public ResourceFactory
{
    virtual ~TextureSamplerFactory() = default;

    struct Root
    {
        CGPUDeviceId device = nullptr;
    };

    float AsyncSerdeLoadFactor() override { return 0.5f; }
    [[nodiscard]] static TextureSamplerFactory* Create(const Root& root);
    static void Destroy(TextureSamplerFactory* factory);
};
} // namespace skr::resource
#endif