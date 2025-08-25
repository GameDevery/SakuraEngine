// EXR coder implementation
#include "SkrCore/log.h"
#include "SkrCore/memory/memory.h"
#include "SkrBase/misc/debug.h"
#include "image_coder_exr.hpp"
#include "image_coder_base.hpp"

#include <cstdint>
#include <cstring>
#include <cctype>
#include "SkrContainersDef/vector.hpp"
#include <SkrContainers/string.hpp>
#include <cfloat>

#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfIO.h>
#include <Imath/ImathBox.h>

namespace skr
{

namespace
{
    struct MemFileOut : public Imf::OStream
    {
        MemFileOut(const char fileName[])
            : Imf::OStream(fileName)
            , Pos(0)
        {
        }

        void write(const char c[], int n) override
        {
            int64_t srcN = (int64_t)n;
            int64_t destPost = Pos + srcN;
            if (destPost > (int64_t)Data.size())
            {
                Data.resize_unsafe((uint64_t)std::max((int64_t)Data.size() * 2, destPost));
            }
            std::memcpy(Data.data() + Pos, c, (size_t)srcN);
            Pos += srcN;
        }

        uint64_t tellp() override { return (uint64_t)Pos; }
        void seekp(uint64_t pos) override { Pos = (int64_t)pos; }

        int64_t Pos;
        skr::Vector<uint8_t> Data;
    };

    struct MemFileIn : public Imf::IStream
    {
        MemFileIn(const void* inData, int64_t inSize)
            : Imf::IStream("")
            , Data((const char*)inData)
            , Size(inSize < 0 ? 0 : (uint64_t)inSize)
            , Pos(0)
        {
            if (inSize < 0)
            {
                throw std::exception();
            }
        }

        bool read(char out[], int n) override
        {
            if (n < 0) throw std::exception();
            uint64_t next = Pos + (uint64_t)n;
            if (next > Size) throw std::exception();
            std::memcpy(out, Data + Pos, (size_t)n);
            Pos = next;
            return Pos != Size;
        }

        uint64_t tellg() override { return Pos; }
        void seekg(uint64_t pos) override { Pos = pos; }
        int64_t size() override { return (int64_t)Size; }

        const char* Data;
        uint64_t Size;
        uint64_t Pos;
    };

    static inline bool IsFiniteFloat(float f)
    {
        return f >= -FLT_MAX && f <= FLT_MAX;
    }
}

// ================== Decoder ==================
struct EXRImageDecoder::Impl
{
    Imath::Box2i DataWindow;
    skr::Vector<skr::String> FileChannels;
    skr::Vector<skr::String> ChosenStrings;
    const char* Chosen[4] = { nullptr, nullptr, nullptr, nullptr };
    int ChannelCount = 0;
};

EXRImageDecoder::EXRImageDecoder() SKR_NOEXCEPT
{
    P = (Impl*)sakura_malloc(sizeof(Impl));
    new (P) Impl();
}

EXRImageDecoder::~EXRImageDecoder() SKR_NOEXCEPT
{
    if (P)
    {
        P->~Impl();
        sakura_free(P);
        P = nullptr;
    }
}

EImageCoderFormat EXRImageDecoder::get_image_format() const SKR_NOEXCEPT
{
    return EImageCoderFormat::IMAGE_CODER_FORMAT_EXR;
}

bool EXRImageDecoder::load_exr_header() SKR_NOEXCEPT
{
    try
    {
        MemFileIn mem(encoded_view.data(), (int64_t)encoded_view.size());
        Imf::InputFile imf(mem);
        const Imf::Header& header = imf.header();
        const Imf::ChannelList& channels = header.channels();
        P->DataWindow = header.dataWindow();
        const uint32_t width = (uint32_t)(P->DataWindow.max.x - P->DataWindow.min.x + 1);
        const uint32_t height = (uint32_t)(P->DataWindow.max.y - P->DataWindow.min.y + 1);

        bool onlyHalf = true;
        int32_t channelCount = 0;
        P->FileChannels.clear();
        for (Imf::ChannelList::ConstIterator it = channels.begin(); it != channels.end(); ++it)
        {
            channelCount++;
            if (it.channel().type != Imf::HALF)
            {
                onlyHalf = false;
            }
            // copy name content we own
            const char* nm = it.name();
            P->FileChannels.add((const char8_t*)nm);
        }

        if (channelCount == 0)
        {
            return false;
        }

        const uint32_t bitDepth = onlyHalf ? 16u : 32u;
        const EImageCoderColorFormat colorFormat = (channelCount == 1) ? IMAGE_CODER_COLOR_FORMAT_GrayF : IMAGE_CODER_COLOR_FORMAT_RGBAF;
        // Decide Chosen channel mapping to match UE behavior (by last char), fallback stuffing
        P->ChannelCount = (colorFormat == IMAGE_CODER_COLOR_FORMAT_GrayF) ? 1 : 4;
        const char* desired = (P->ChannelCount == 1) ? "Y" : "RGBA";
        for (int i = 0; i < 4; ++i) P->Chosen[i] = nullptr;

        P->ChosenStrings.clear();
        P->ChosenStrings.resize_default(P->ChannelCount);
        skr::Vector<int> chosenSet;
        chosenSet.resize_default(P->ChannelCount);
        for (int j = 0; j < P->ChannelCount; ++j) chosenSet[j] = 0;

        // First pass: by last letter
        skr::Vector<int> used;
        used.resize_zeroed((int)P->FileChannels.size());
        for (int i = 0; i < (int)P->FileChannels.size(); ++i) used[i] = 0;
        for (int i = 0; i < (int)P->FileChannels.size(); ++i)
        {
            const auto& s = P->FileChannels[i];
            const char* nm = (const char*)s.c_str();
            size_t len = std::strlen(nm);
            char last = (len > 0) ? (char)std::toupper(nm[len - 1]) : 0;
            for (int j = 0; j < P->ChannelCount; ++j)
            {
                if (!chosenSet[j] && last == desired[j])
                {
                    P->ChosenStrings[j] = (const char8_t*)nm;
                    chosenSet[j] = 1;
                    used[i] = 1;
                    break;
                }
            }
        }
        // Second pass: fill remaining in order
        int fillJ = 0;
        for (int i = 0; i < (int)P->FileChannels.size(); ++i)
        {
            if (used[i]) continue;
            while (fillJ < P->ChannelCount && chosenSet[fillJ]) ++fillJ;
            if (fillJ >= P->ChannelCount) break;
            P->ChosenStrings[fillJ].assign((const char8_t*)P->FileChannels[i].c_str());
            chosenSet[fillJ] = 1;
        }
        // Third pass: defaults
        static const char8_t* defaults[4] = {u8"default.R",u8"default.G",u8"default.B",u8"default.A"};
        for (int j = 0; j < P->ChannelCount; ++j)
        {
            if (!chosenSet[j])
            {
                P->ChosenStrings[j].assign(defaults[j]);
                chosenSet[j] = 1;
            }
        }

        setRawProps(width, height, colorFormat, bitDepth);
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool EXRImageDecoder::initialize(const uint8_t* data, uint64_t size) SKR_NOEXCEPT
{
    if (!BaseImageDecoder::initialize(data, size))
        return false;
    return load_exr_header();
}

bool EXRImageDecoder::decode(EImageCoderColorFormat in_format, uint32_t in_bit_depth) SKR_NOEXCEPT
{
    SKR_ASSERT(initialized);
    if (!(in_format == IMAGE_CODER_COLOR_FORMAT_RGBAF || in_format == IMAGE_CODER_COLOR_FORMAT_GrayF))
    {
        SKR_LOG_ERROR(u8"Unsupported EXR output format. Use RGBAF or GrayF.");
        return false;
    }
    if (!(in_bit_depth == 16 || in_bit_depth == 32))
    {
        SKR_LOG_ERROR(u8"Unsupported EXR bit depth. Use 16 or 32.");
        return false;
    }

    const uint32_t width = get_width();
    const uint32_t height = get_height();
    const bool isGray = (in_format == IMAGE_CODER_COLOR_FORMAT_GrayF);
    const int channelCount = isGray ? 1 : 4;

    try
    {
        MemFileIn mem(encoded_view.data(), (int64_t)encoded_view.size());
        Imf::InputFile imf(mem);
        const Imf::Header& header = imf.header();
        Imath::Box2i dataWindow = P->DataWindow; // cached

        Imf::FrameBuffer fb;

        const Imf::PixelType exrType = (in_bit_depth == 16) ? Imf::HALF : Imf::FLOAT;
        const int bytesPerChan = (exrType == Imf::HALF) ? 2 : 4;

        skr::Vector<skr::Vector<uint8_t>> channels;
        channels.resize_default(channelCount);
        const int64_t bytesPerChannel = (int64_t)bytesPerChan * width * height;
        for (int c = 0; c < channelCount; ++c)
        {
            channels[c].resize_unsafe((size_t)bytesPerChannel);
        }
        // Use cached mapping (point to owned strings)
        for (int i = 0; i < channelCount; ++i)
        {
            P->Chosen[i] = (const char*)P->ChosenStrings[i].c_str();
        }
        const char* names[4] = { P->Chosen[0], P->Chosen[1], P->Chosen[2], P->Chosen[3] };

        for (int c = 0; c < channelCount; ++c)
        {
            // alpha默认值与 UE 一致：只有在 RGBA 模式且通道名末字母为 'A' 且落在槽 3 时，才用 1.0；其余 0.0
            bool isAlpha = false;
            if (!isGray && c == 3)
            {
                const char* nm = names[c];
                size_t len = std::strlen(nm);
                char last = (len > 0) ? (char)std::toupper(nm[len - 1]) : 0;
                isAlpha = (last == 'A');
            }
            const double defVal = isAlpha ? 1.0 : 0.0;

            const int64_t dataWindowOffset = ((int64_t)dataWindow.min.x + (int64_t)dataWindow.min.y * width) * bytesPerChan;
            char* base = (char*)((intptr_t)channels[c].data() - dataWindowOffset);
            fb.insert(names[c], Imf::Slice(exrType, base, bytesPerChan, (size_t)bytesPerChan * width, 1, 1, defVal));
        }

        imf.setFrameBuffer(fb);
        imf.readPixels(dataWindow.min.y, dataWindow.max.y);

        decoded_size = (uint64_t)width * height * channelCount * bytesPerChan;
        decoded_data = EXRImageDecoder::Allocate(decoded_size, get_alignment());

        if (in_bit_depth == 16)
        {
            uint8_t* out = decoded_data;
            for (int64_t off = 0; off < bytesPerChannel; off += 2)
            {
                for (int c = 0; c < channelCount; ++c)
                {
                    const uint8_t* src = &channels[c][(size_t)off];
                    *out++ = src[0];
                    *out++ = src[1];
                }
            }
        }
        else
        {
            float* out = reinterpret_cast<float*>(decoded_data);
            for (int64_t off = 0; off < bytesPerChannel; off += 4)
            {
                for (int c = 0; c < channelCount; ++c)
                {
                    float v;
                    std::memcpy(&v, &channels[c][(size_t)off], 4);
                    if (!IsFiniteFloat(v))
                    {
                        if (v > FLT_MAX) v = FLT_MAX; else if (v < -FLT_MAX) v = -FLT_MAX; else v = 0.0f;
                    }
                    *out++ = v;
                }
            }
        }
        return true;
    }
    catch (const std::exception& e)
    {
        SKR_LOG_ERROR(u8"EXR decode failed: %s", (const char8_t*)e.what());
        return false;
    }
}

// ================== Encoder ==================
EXRImageEncoder::EXRImageEncoder() SKR_NOEXCEPT
{
}

EXRImageEncoder::~EXRImageEncoder() SKR_NOEXCEPT
{
}

EImageCoderFormat EXRImageEncoder::get_image_format() const SKR_NOEXCEPT
{
    return EImageCoderFormat::IMAGE_CODER_FORMAT_EXR;
}

bool EXRImageEncoder::encode() SKR_NOEXCEPT
{
    SKR_ASSERT(initialized);

    const auto width = get_width();
    const auto height = get_height();
    const auto color = get_color_format();
    const auto bitDepth = get_bit_depth();

    if (!((color == IMAGE_CODER_COLOR_FORMAT_RGBAF || color == IMAGE_CODER_COLOR_FORMAT_GrayF) && (bitDepth == 16 || bitDepth == 32)))
    {
        SKR_LOG_ERROR(u8"EXR encode expects GrayF/RGBAF with 16 or 32-bit.");
        return false;
    }

    const bool isGray = (color == IMAGE_CODER_COLOR_FORMAT_GrayF);
    const int channelCount = isGray ? 1 : 4;
    const Imf::PixelType exrType = (bitDepth == 16) ? Imf::HALF : Imf::FLOAT;
    const int bytesPerChan = (exrType == Imf::HALF) ? 2 : 4;

    const int64_t planeSize = (int64_t)bytesPerChan * width * height;

    skr::Vector<skr::Vector<uint8_t>> channels;
    channels.resize_default(channelCount);
    for (int c = 0; c < channelCount; ++c)
        channels[c].resize_unsafe((size_t)planeSize);

    const uint8_t* src = decoded_view.data();
    for (int64_t offNI = 0, offI = 0; offI < (int64_t)decoded_view.size(); offNI += bytesPerChan)
    {
        for (int c = 0; c < channelCount; ++c)
        {
            std::memcpy(&channels[c][(size_t)offNI], src + offI, (size_t)bytesPerChan);
            offI += bytesPerChan;
        }
    }
    
    try
    {
        Imf::Header header((int)width, (int)height);
        header.compression() = Imf::ZIP_COMPRESSION;

        Imf::FrameBuffer fb;
        static const char* namesRGBA[4] = {"R","G","B","A"};
        static const char* nameGray[1] = {"Y"};
        const char* const* names = isGray ? nameGray : namesRGBA;

        for (int c = 0; c < channelCount; ++c)
        {
            header.channels().insert(names[c], Imf::Channel(exrType));
        }
        for (int c = 0; c < channelCount; ++c)
        {
            fb.insert(names[c], Imf::Slice(exrType, (char*)channels[c].data(), bytesPerChan, (size_t)bytesPerChan * width));
        }

        MemFileOut mem("");
        int64_t fileLen = 0;
        {
            Imf::OutputFile out(mem, header);
            out.setFrameBuffer(fb);
            out.writePixels(height);
            fileLen = (int64_t)mem.tellp();
        }

        encoded_size = (uint64_t)fileLen;
        encoded_data = EXRImageEncoder::Allocate(encoded_size, get_alignment());
        std::memcpy(encoded_data, mem.Data.data(), (size_t)encoded_size);
        return true;
    }
    catch (const std::exception& e)
    {
        SKR_LOG_ERROR(u8"EXR encode failed: %s", (const char8_t*)e.what());
        return false;
    }
}

} // namespace skr