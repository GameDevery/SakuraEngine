namespace SB.Core
{
    public enum Architecture
    {
        X86,
        X64,
        ARM64
    }

    public enum SIMDArchitecture
    {
        SSE2,
        SSE4_2,
        AVX,
        AVX512,
        AVX10_1,
        Neon
    }

    public enum OSPlatform
    {
        Windows,
        Linux,
        OSX
    }
}
