struct VSOut
{
    // ignore SV_POSITION in pixel shader if we dont use it
    // float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    centroid float4 normal : NORMAL;
    centroid float4 tangent : TANGENT;
};


void main(VSOut psIn,     
    out float4 o_color : SV_Target0) : SV_TARGET
{
    float2 uv = psIn.uv;
    o_color = 0.5f + 0.5f * psIn.normal;
}