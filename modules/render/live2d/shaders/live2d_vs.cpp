#include "live2d_cb.hpp"

struct [[stage_inout]] VSIn
{
    float2 POSITION;
    float2 TEXCOORD0;
};

struct [[stage_inout]] VSOut
{
    float4 TEXCOORD0;
    float2 TEXCOORD1;
    float4 SV_Position;
};

[[vertex_shader("vertex_shader")]]
VSOut vertex(VSIn input)
{
    VSOut output;
    output.SV_Position = float4(input.POSITION, 0.f, 1.f) * push_constants.projection_matrix;
    output.TEXCOORD0 = float4(input.POSITION, 0.f, 1.f) * push_constants.clip_matrix;
    output.TEXCOORD1.x = input.TEXCOORD0.x;
    output.TEXCOORD1.y = 1.f - input.TEXCOORD0.y;
    return output;
}