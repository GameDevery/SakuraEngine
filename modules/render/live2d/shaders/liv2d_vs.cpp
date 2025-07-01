#include "live2d_cb.hpp"

struct [[stage_inout]] VSIn
{
    float2 pos;
    float2 uv;
};

struct [[stage_inout]] VSOut
{
    float4 clip_pos;
    float2 uv;
};

[[vertex_shader("vertex_shader")]]
VSOut vertex(VSIn input, [[builtin("Position")]] float4& position)
{
    VSOut output;
    position = push_constants.projection_matrix * float4(input.pos, 0.f, 1.f);
    output.clip_pos = push_constants.clip_matrix * float4(input.pos, 0.f, 1.f);
    output.uv.x = input.uv.x;
    output.uv.y = 1.f - input.uv.y;
    return output;
}