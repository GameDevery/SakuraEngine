namespace skr::CppSL::HLSL
{
const wchar_t* kHLSLTextureIntrinsics = LR"__de___l___im__(
template <typename T> T texture_read(Texture2D<T> tex, uint2 loc) { return tex.Load(uint3(loc, 0)); }
template <typename T> T texture_read(RWTexture2D<T> tex, uint2 loc) { return tex.Load(uint3(loc, 0)); }
template <typename T> T texture_read(Texture2D<T> tex, uint3 loc_and_mip) { return tex.Load(loc_and_mip); }
template <typename T> T texture_read(RWTexture2D<T> tex, uint3 loc_and_mip) { return tex.Load(loc_and_mip); }
template <typename T> T texture_write(RWTexture2D<T> tex, uint2 uv, T v) { return tex[uv] = v; }

template <typename T> uint2 texture_size(Texture2D<T> tex) { uint Width, Height, Mips; tex.GetDimensions(0, Width, Height, Mips); return uint2(Width, Height); }
template <typename T> uint3 texture_size(Texture2DArray<T> tex) { uint Width, Height, Length; tex.GetDimensions(Width, Height, Length); return uint3(Width, Height, Length); }
template <typename T> uint3 texture_size(RWTexture2DArray<T> tex) { uint Width, Height, Length; tex.GetDimensions(Width, Height, Length); return uint3(Width, Height, Length); }
template <typename T> uint2 texture_size(RWTexture2D<T> tex) { uint Width, Height; tex.GetDimensions(Width, Height); return uint2(Width, Height); }
template <typename T> uint3 texture_size(Texture3D<T> tex) { uint Width, Height, Depth, Mips; tex.GetDimensions(0, Width, Height, Depth, Mips); return uint3(Width, Height, Depth); }
template <typename T> uint3 texture_size(RWTexture3D<T> tex) { uint Width, Height, Depth; tex.GetDimensions(Width, Height, Depth); return uint3(Width, Height, Depth); }

float4 texture_sample(Texture2D t, SamplerState s, float2 uv) { return t.Sample(s, uv); }
float4 texture_sample(Texture3D t, SamplerState s, float3 uv) { return t.Sample(s, uv); }
float4 texture_sample(TextureCube t, SamplerState s, float3 uv) { return t.Sample(s, uv); }

float4 texture_sample_level(Texture2D t, SamplerState s, float2 uv, uint level) { return t.SampleLevel(s, uv, level); }
float4 texture_sample_level(Texture3D t, SamplerState s, float3 uv, uint level) { return t.SampleLevel(s, uv, level); }
float4 texture_sample_level(TextureCube t, SamplerState s, float3 uv, uint level) { return t.SampleLevel(s, uv, level); }
)__de___l___im__";

} // namespace CppSL::HLSL