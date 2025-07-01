#pragma once
#include <std/std.hpp>
#include <std2/constant_buffer.hpp>

using namespace skr::shader;

struct Constants
{
    float4x4 projection_matrix;
    float4x4 clip_matrix;
    float4 base_color;
    float4 multiply_color;
    float4 screen_color;
    float4 channel_flag;
    float use_mask;
    float pad0;
    float pad1;
    float pad2;
};

extern ConstantBuffer<Constants>& push_constants;