
struct VSIn
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct VSOut
{
    float3 color : COLOR;
};

VSOut main(const VSIn input, out float4 position : SV_POSITION)
{
    VSOut output;
    output.color = input.color;
    position = float4(input.position, 1.0f);
    return output;
}