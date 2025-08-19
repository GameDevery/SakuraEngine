#pragma once
#include "SkrRenderer/fwd_types.h"
#include "SkrGraphics/flags.h"
#include "SkrRTTR/enum_tools.hpp"
#ifndef __meta__
    #include "SkrRenderer/shader_hash.generated.h" // IWYU pragma: export
#endif

namespace skr {
sreflect_struct(
    guid = "5a54720c-34b2-444c-8e3a-5977c94136c3";
    serde = @bin | @json;)
StableShaderHash
{
    uint32_t valuea = 0;
    uint32_t valueb = 0;
    uint32_t valuec = 0;
    uint32_t valued = 0;

#ifdef __cplusplus
    inline constexpr StableShaderHash() = default;

    inline bool operator==(const StableShaderHash& other) const
    {
        return valuea == other.valuea && valueb == other.valueb && valuec == other.valuec && valued == other.valued;
    }

    struct hasher
    {
        SKR_RENDERER_API size_t operator()(const StableShaderHash& hash) const;
    };

    SKR_RENDERER_API static StableShaderHash hash_string(const char* str, uint32_t size) SKR_NOEXCEPT;

    SKR_RENDERER_API static StableShaderHash from_string(const char* str) SKR_NOEXCEPT;

    SKR_RENDERER_API StableShaderHash(uint32_t a, uint32_t b, uint32_t c, uint32_t d) SKR_NOEXCEPT;
    SKR_RENDERER_API explicit operator skr::String() const SKR_NOEXCEPT;
#endif
};
static SKR_CONSTEXPR StableShaderHash kZeroStableShaderHash = StableShaderHash();

sreflect_struct(
    guid = "0291f512-747e-4b64-ba5c-5fdc412220a3";
    serde = @bin | @json;)
PlatformShaderHash
{
    uint32_t flags;
    uint32_t encoded_digits[4];

#ifdef __cplusplus
    inline bool operator==(const PlatformShaderHash& other) const
    {
        return encoded_digits[0] == other.encoded_digits[0] && encoded_digits[1] == other.encoded_digits[1] && encoded_digits[2] == other.encoded_digits[2] && encoded_digits[3] == other.encoded_digits[3] && flags == other.flags;
    }

    struct hasher
    {
        SKR_RENDERER_API size_t operator()(const PlatformShaderHash& hash) const;
    };
#endif
};

sreflect_struct(
    guid = "b0b69898-166f-49de-a675-7b04405b98b1";
    serde = @bin | @json;)
PlatformShaderIdentifier
{
#ifdef __cplusplus
    skr::EnumAsValue<ECGPUShaderBytecodeType> bytecode_type;
    skr::EnumAsValue<ECGPUShaderStage> shader_stage;
#else
    ECGPUShaderBytecodeType bytecode_type;
    ECGPUShaderStage shader_stage;
#endif
    PlatformShaderHash hash;

#ifdef __cplusplus
    inline bool operator==(const PlatformShaderIdentifier& other) const
    {
        return hash == other.hash && bytecode_type == other.bytecode_type && shader_stage == other.shader_stage;
    }

    struct hasher
    {
        SKR_RENDERER_API size_t operator()(const PlatformShaderIdentifier& hash) const;
    };
#endif
};
} // namespace skr