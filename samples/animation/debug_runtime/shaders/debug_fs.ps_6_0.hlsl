struct VSOut
{
    float3 color : COLOR;
};

void main(VSOut psIn,     
    out float4 o_color : SV_Target0) : SV_TARGET
{
    o_color = float4(psIn.color, 1.0f);
}