#pragma once
#include "image_coder_base.hpp"

namespace skr
{

struct SKR_IMAGE_CODER_API HDRImageDecoder : public BaseImageDecoder
{
public:
    HDRImageDecoder() SKR_NOEXCEPT;
    ~HDRImageDecoder() SKR_NOEXCEPT;

    EImageCoderFormat get_image_format() const SKR_NOEXCEPT final;
    bool              initialize(const uint8_t* data, uint64_t size) SKR_NOEXCEPT final;
    bool              decode(EImageCoderColorFormat format, uint32_t bit_depth) SKR_NOEXCEPT final;

private:
    bool parse_header() SKR_NOEXCEPT;
    bool get_header_line(const uint8_t*& cursor, char line[256]) const SKR_NOEXCEPT;
    static bool parse_match_string(const char*& cursor, const char* expected) SKR_NOEXCEPT;
    static bool parse_positive_int(const char*& cursor, int* outValue) SKR_NOEXCEPT;
    static bool parse_image_size(const char* line, int* outWidth, int* outHeight) SKR_NOEXCEPT;
    static bool have_bytes(const uint8_t* cursor, const uint8_t* end, int amount) SKR_NOEXCEPT;
    bool decompress_scanline(uint8_t* out, const uint8_t*& in, const uint8_t* end) const SKR_NOEXCEPT;
    bool old_decompress_scanline(uint8_t* out, const uint8_t*& in, const uint8_t* end, int length, bool initialRunAllowed) const SKR_NOEXCEPT;

private:
    const uint8_t* rgb_data_start = nullptr;
};

struct SKR_IMAGE_CODER_API HDRImageEncoder : public BaseImageEncoder
{
public:
    HDRImageEncoder() SKR_NOEXCEPT;
    ~HDRImageEncoder() SKR_NOEXCEPT;

    EImageCoderFormat get_image_format() const SKR_NOEXCEPT final;
    bool              encode() SKR_NOEXCEPT final;
};

} // namespace skr


