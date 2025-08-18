#pragma once
#include "SkrBase/misc/hash.h"
#include "SkrRenderer/resources/shader_meta_resource.hpp"
#include "SkrRenderer/resources/shader_resource.hpp"

namespace option_utils
{
inline void stringfy(skr::String& string, skr::span<skr_shader_option_instance_t> ordered_options)
{
    for (auto&& option : ordered_options)
    {
        string.append(option.key);
        string.append(u8"=");
        string.append(option.value);
        string.append(u8";");
    }
}

inline void stringfy(skr::String& string, const skr_shader_option_sequence_t& seq, skr::span<uint32_t> indices)
{
    for (uint32_t i = 0; i < seq.keys.size(); i++)
    {
        const auto selection = indices[i];
        string.append(seq.keys[i]);
        string.append(u8"=");
        string.append(seq.values[i][selection]);
        string.append(u8";");
    }
}
}