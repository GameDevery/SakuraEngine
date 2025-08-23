#include "SkrCore/log.h"
#include "SkrBase/misc/debug.h"
#include "SkrCore/memory/memory.h"
#include "image_coder_hdr.hpp"

#include <cstdint>
#include <cstring>
#include <cfloat>

namespace skr
{

static inline bool HDR_HaveBytes(const uint8_t* cursor, const uint8_t* end, int amount)
{
    return end - cursor >= amount;
}

HDRImageDecoder::HDRImageDecoder() SKR_NOEXCEPT {}
HDRImageDecoder::~HDRImageDecoder() SKR_NOEXCEPT {}

EImageCoderFormat HDRImageDecoder::get_image_format() const SKR_NOEXCEPT
{
    return EImageCoderFormat::IMAGE_CODER_FORMAT_HDR;
}

bool HDRImageDecoder::get_header_line(const uint8_t*& cursor, char line[256]) const SKR_NOEXCEPT
{
    const uint8_t* end = encoded_view.data() + encoded_view.size();
    for (int i = 0; i < 256; ++i)
    {
        if (cursor > end) return false;
        if (cursor >= end) return false;
        uint8_t c = *cursor;
        if (c == 0) return false;
        if (c == '\n')
        {
            ++cursor;
            line[i] = 0;
            return true;
        }
        line[i] = (char)c;
        ++cursor;
    }
    return false;
}

bool HDRImageDecoder::parse_match_string(const char*& p, const char* expected) SKR_NOEXCEPT
{
    const char* s = expected;
    const char* c = p;
    while (*s)
    {
        if (*c != *s) return false;
        ++c; ++s;
    }
    p = c;
    return true;
}

bool HDRImageDecoder::parse_positive_int(const char*& p, int* outValue) SKR_NOEXCEPT
{
    if (*p < '0' || *p > '9') return false;
    int v = *p - '0';
    ++p;
    while (*p >= '0' && *p <= '9')
    {
        int d = *p - '0';
        ++p;
        int64_t nv = (int64_t)v * 10 + d;
        if (nv > INT32_MAX) return false;
        v = (int)nv;
    }
    *outValue = v;
    return true;
}

bool HDRImageDecoder::parse_image_size(const char* line, int* outWidth, int* outHeight) SKR_NOEXCEPT
{
    if (!parse_match_string(line, "-Y ")) return false;
    if (!parse_positive_int(line, outHeight)) return false;
    if (!parse_match_string(line, " +X ")) return false;
    if (!parse_positive_int(line, outWidth)) return false;
    return *line == 0;
}

bool HDRImageDecoder::parse_header() SKR_NOEXCEPT
{
    if (encoded_view.size() < 11) return false;
    const uint8_t* p = encoded_view.data();
    char line[256];

    if (!get_header_line(p, line)) return false;
    if (std::strcmp(line, "#?RADIANCE") != 0 && std::strcmp(line, "#?RGBE") != 0)
        return false;

    bool hasFormat = false;
    for (;;)
    {
        if (!get_header_line(p, line)) return false;
        if (!line[0]) break;
        const char* cur = line;
        if (parse_match_string(cur, "FORMAT="))
        {
            hasFormat = true;
            if (std::strcmp(cur, "32-bit_rle_rgbe") != 0)
            {
                return false;
            }
        }
    }
    if (!hasFormat) return false;

    if (!get_header_line(p, line)) return false;
    int w=0,h=0;
    if (!parse_image_size(line, &w, &h) || w <= 0 || h <= 0) return false;

    setRawProps((uint32_t)w, (uint32_t)h, IMAGE_CODER_COLOR_FORMAT_BGRE, 8);
    rgb_data_start = p;
    return true;
}

bool HDRImageDecoder::initialize(const uint8_t* data, uint64_t size) SKR_NOEXCEPT
{
    if (!BaseImageDecoder::initialize(data, size)) return false;
    return parse_header();
}

bool HDRImageDecoder::have_bytes(const uint8_t* cursor, const uint8_t* end, int amount) SKR_NOEXCEPT
{
    return HDR_HaveBytes(cursor, end, amount);
}

bool HDRImageDecoder::old_decompress_scanline(uint8_t* Out, const uint8_t*& InCodedScanline, const uint8_t* InEnd, int length, bool initialRunAllowed) const SKR_NOEXCEPT
{
    const uint8_t* In = InCodedScanline;
    int shift = initialRunAllowed ? 0 : 32;
    while (length > 0)
    {
        if (!HDR_HaveBytes(In, InEnd, 4)) return false;
        uint8_t R = *In++;
        uint8_t G = *In++;
        uint8_t B = *In++;
        uint8_t E = *In++;
        if (R == 1 && G == 1 && B == 1)
        {
            if (shift >= 32) return false;
            int64_t count = (int64_t)E << shift;
            if (count > length) return false;
            length -= (int)count;
            R = *(Out - 4);
            G = *(Out - 3);
            B = *(Out - 2);
            E = *(Out - 1);
            while (count-- > 0)
            {
                *Out++ = B;
                *Out++ = G;
                *Out++ = R;
                *Out++ = E;
            }
            shift += 8;
        }
        else
        {
            *Out++ = B;
            *Out++ = G;
            *Out++ = R;
            *Out++ = E;
            shift = 0;
            --length;
        }
    }
    InCodedScanline = In;
    return true;
}

bool HDRImageDecoder::decompress_scanline(uint8_t* Out, const uint8_t*& In, const uint8_t* InEnd) const SKR_NOEXCEPT
{
    const int MINELEN = 8;
    const int MAXELEN = 0x7fff;
    const int width = (int)get_width();
    if (width < MINELEN || width > MAXELEN)
    {
        return old_decompress_scanline(Out, In, InEnd, width, false);
    }
    if (!HDR_HaveBytes(In, InEnd, 1)) return false;
    uint8_t Red = *In;
    if (Red != 2)
    {
        return old_decompress_scanline(Out, In, InEnd, width, false);
    }
    ++In;
    if (!HDR_HaveBytes(In, InEnd, 3)) return false;
    uint8_t Green = *In++;
    uint8_t Blue = *In++;
    uint8_t Exponent = *In++;
    if (Green != 2 || (Blue & 128))
    {
        *Out++ = Blue; *Out++ = Green; *Out++ = Red; *Out++ = Exponent;
        return old_decompress_scanline(Out, In, InEnd, width - 1, true);
    }
    for (uint32_t chan = 0; chan < 4; ++chan)
    {
        uint8_t writeOffset = (chan == 0) ? 2 : (chan == 2) ? 0 : chan; // BGRE order
        const uint8_t* localIn = In;
        uint8_t* outPtr = Out + writeOffset;
        int x = 0;
        while (x < width)
        {
            if (!HDR_HaveBytes(localIn, InEnd, 1)) return false;
            uint8_t cur = *localIn++;
            if (cur > 128)
            {
                int count = cur & 0x7f;
                if (!HDR_HaveBytes(localIn, InEnd, 1)) return false;
                uint8_t value = *localIn++;
                if (width - x < count) return false;
                for (int i = 0; i < count; ++i)
                {
                    *outPtr = value;
                    outPtr += 4;
                }
                x += count;
            }
            else
            {
                int count = cur;
                if (!HDR_HaveBytes(localIn, InEnd, count)) return false;
                if (width - x < count) return false;
                for (int i = 0; i < count; ++i)
                {
                    *outPtr = *localIn++;
                    outPtr += 4;
                }
                x += count;
            }
        }
        In = localIn;
    }
    return true;
}

bool HDRImageDecoder::decode(EImageCoderColorFormat format, uint32_t bit_depth) SKR_NOEXCEPT
{
    SKR_ASSERT(initialized);
    const uint32_t width = get_width();
    const uint32_t height = get_height();

    // Path 1: BGRE 8-bit (RGBE) raw output (UE style)
    if (format == IMAGE_CODER_COLOR_FORMAT_BGRE && bit_depth == 8)
    {
        const int64_t size = (int64_t)width * height * 4;
        decoded_size = size;
        decoded_data = HDRImageDecoder::Allocate(decoded_size, get_alignment());

        const uint8_t* p = rgb_data_start;
        const uint8_t* end = encoded_view.data() + encoded_view.size();
        for (uint32_t y = 0; y < height; ++y)
        {
            if (!decompress_scanline(&decoded_data[(int64_t)width * y * 4], p, end))
            {
                return false;
            }
        }
        return true;
    }

    // Path 2: Convert RGBE -> linear float RGBAF (16/32)
    if (format == IMAGE_CODER_COLOR_FORMAT_RGBAF && (bit_depth == 16 || bit_depth == 32))
    {
        const uint8_t* p = rgb_data_start;
        const uint8_t* end = encoded_view.data() + encoded_view.size();

        // Temp scanline buffer in BGRE (RGBE)
        skr::Vector<uint8_t> scanline;
        scanline.resize_unsafe((size_t)width * 4);

        auto ldexp_int = [](int e)->float { return ldexpf(1.0f, e); };
        auto to_scale = [&](uint8_t E)->float {
            if (E == 0) return 0.0f;
            // value = mantissa/256 * 2^(E-128) = mantissa * 2^(E-128-8)
            return ldexp_int((int)E - 128 - 8);
        };

        if (bit_depth == 32)
        {
            const int64_t outBytes = (int64_t)width * height * 4 * 4;
            decoded_size = outBytes;
            decoded_data = HDRImageDecoder::Allocate(decoded_size, get_alignment());
            float* out = reinterpret_cast<float*>(decoded_data);

            for (uint32_t y = 0; y < height; ++y)
            {
                uint8_t* row = scanline.data();
                if (!decompress_scanline(row, p, end))
                {
                    return false;
                }
                for (uint32_t x = 0; x < width; ++x)
                {
                    const uint8_t B = row[x*4 + 0];
                    const uint8_t G = row[x*4 + 1];
                    const uint8_t R = row[x*4 + 2];
                    const uint8_t E = row[x*4 + 3];
                    if (E == 0)
                    {
                        *out++ = 0.0f;
                        *out++ = 0.0f;
                        *out++ = 0.0f;
                        *out++ = 1.0f;
                    }
                    else
                    {
                        const float s = to_scale(E);
                        float rf = (float)R * s;
                        float gf = (float)G * s;
                        float bf = (float)B * s;
                        // sanitize
                        rf = (rf >= 0 && rf <= FLT_MAX) ? rf : (rf < 0 ? 0.0f : FLT_MAX);
                        gf = (gf >= 0 && gf <= FLT_MAX) ? gf : (gf < 0 ? 0.0f : FLT_MAX);
                        bf = (bf >= 0 && bf <= FLT_MAX) ? bf : (bf < 0 ? 0.0f : FLT_MAX);
                        *out++ = rf;
                        *out++ = gf;
                        *out++ = bf;
                        *out++ = 1.0f;
                    }
                }
            }
            return true;
        }
        else // bit_depth == 16
        {
            auto float_to_half = [](float f)->uint16_t {
                // IEEE 754 float to half conversion (round-to-nearest-even)
                union { uint32_t u; float f; } v = {0}; v.f = f;
                uint32_t x = v.u;
                uint32_t sign = (x >> 16) & 0x8000u;
                uint32_t mant = x & 0x007FFFFFu;
                int32_t exp = (int32_t)((x >> 23) & 0xFF) - 127 + 15;
                if (exp <= 0)
                {
                    if (exp < -10) return (uint16_t)sign; // underflow -> 0
                    mant |= 0x00800000u;
                    uint32_t t = (uint32_t)(14 - exp);
                    uint32_t halfMant = mant >> (t + 1);
                    // round
                    if ((mant >> t) & 1u) halfMant += 1;
                    return (uint16_t)(sign | halfMant);
                }
                else if (exp >= 31)
                {
                    // overflow -> inf
                    return (uint16_t)(sign | 0x7C00u);
                }
                uint16_t half = (uint16_t)(sign | (uint16_t)(exp << 10) | (uint16_t)(mant >> 13));
                // round: check lowest discarded bit
                if (mant & 0x00001000u) half += 1;
                return half;
            };

            const int64_t outBytes = (int64_t)width * height * 4 * 2;
            decoded_size = outBytes;
            decoded_data = HDRImageDecoder::Allocate(decoded_size, get_alignment());
            uint16_t* out = reinterpret_cast<uint16_t*>(decoded_data);

            for (uint32_t y = 0; y < height; ++y)
            {
                uint8_t* row = scanline.data();
                if (!decompress_scanline(row, p, end))
                {
                    return false;
                }
                for (uint32_t x = 0; x < width; ++x)
                {
                    const uint8_t B = row[x*4 + 0];
                    const uint8_t G = row[x*4 + 1];
                    const uint8_t R = row[x*4 + 2];
                    const uint8_t E = row[x*4 + 3];
                    if (E == 0)
                    {
                        *out++ = 0; *out++ = 0; *out++ = 0; *out++ = float_to_half(1.0f);
                    }
                    else
                    {
                        const float s = to_scale(E);
                        float rf = (float)R * s;
                        float gf = (float)G * s;
                        float bf = (float)B * s;
                        if (!(rf >= 0)) rf = 0; if (!(gf >= 0)) gf = 0; if (!(bf >= 0)) bf = 0;
                        *out++ = float_to_half(rf);
                        *out++ = float_to_half(gf);
                        *out++ = float_to_half(bf);
                        *out++ = float_to_half(1.0f);
                    }
                }
            }
            return true;
        }
    }

    SKR_LOG_ERROR(u8"HDR decoder unsupported output format/bitdepth.");
    return false;
}

HDRImageEncoder::HDRImageEncoder() SKR_NOEXCEPT {}
HDRImageEncoder::~HDRImageEncoder() SKR_NOEXCEPT {}

EImageCoderFormat HDRImageEncoder::get_image_format() const SKR_NOEXCEPT
{
    return EImageCoderFormat::IMAGE_CODER_FORMAT_HDR;
}

bool HDRImageEncoder::encode() SKR_NOEXCEPT
{
    SKR_ASSERT(initialized);
    if (get_color_format() != IMAGE_CODER_COLOR_FORMAT_BGRE || get_bit_depth() != 8)
    {
        SKR_LOG_ERROR(u8"HDR encoder expects BGRE 8-bit raw input.");
        return false;
    }

    const int64_t numPixels = (int64_t)get_width() * get_height();
    char header[128];
    int headerLen = std::snprintf(header, sizeof(header), "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y %u +X %u\n", get_height(), get_width());
    if (headerLen <= 0) return false;

    const int64_t outSize = headerLen + numPixels * 4;
    encoded_data = HDRImageEncoder::Allocate(outSize, get_alignment());
    encoded_size = outSize;
    uint8_t* out = encoded_data;
    std::memcpy(out, header, headerLen);
    out += headerLen;

    const uint8_t* src = decoded_view.data();
    for (int64_t i = 0; i < numPixels; ++i)
    {
        // BGRE -> RGBE
        out[i * 4 + 0] = src[i * 4 + 2];
        out[i * 4 + 1] = src[i * 4 + 1];
        out[i * 4 + 2] = src[i * 4 + 0];
        out[i * 4 + 3] = src[i * 4 + 3];
    }
    return true;
}

} // namespace skr


