#pragma once
#include "image_coder_base.hpp"

namespace skr
{

struct SKR_IMAGE_CODER_API EXRImageDecoder : public BaseImageDecoder
{
public:
    EXRImageDecoder() SKR_NOEXCEPT;
    ~EXRImageDecoder() SKR_NOEXCEPT;

    EImageCoderFormat get_image_format() const SKR_NOEXCEPT final;
    bool              initialize(const uint8_t* data, uint64_t size) SKR_NOEXCEPT final;
    bool              decode(EImageCoderColorFormat format, uint32_t bit_depth) SKR_NOEXCEPT final;

private:
    bool load_exr_header() SKR_NOEXCEPT;

    struct Impl;
    Impl* P = nullptr;
};

struct SKR_IMAGE_CODER_API EXRImageEncoder : public BaseImageEncoder
{
public:
    EXRImageEncoder() SKR_NOEXCEPT;
    ~EXRImageEncoder() SKR_NOEXCEPT;

    EImageCoderFormat get_image_format() const SKR_NOEXCEPT final;
    bool              encode() SKR_NOEXCEPT final;
};

} // namespace skr
