// https://github.com/pm4rtx/bit_cast_hlsl 2fa3ab6
namespace skr::CppSL::HLSL
{
const wchar_t* kHLSLBitCast = LR"__de___l___im__(
#define BITCAST_16BIT_SUPPORTED 0
#define BITCAST_AUX_BASE_TYPE 1
template<typename TDst, typename TSrc>
struct TBitCastTypeIsSame { static const bool Result = false; };
template<typename T>
struct TBitCastTypeIsSame<T, T> { static const bool Result = true; };
template<typename T>
struct TBitCastBaseTypeIs16Bit
{ 
#if BITCAST_16BIT_SUPPORTED
    static const bool Result = TBitCastTypeIsSame<T, float16_t>::Result
                            || TBitCastTypeIsSame<T,  uint16_t>::Result
                            || TBitCastTypeIsSame<T,   int16_t>::Result;
#else
    static const bool Result = false;
#endif
};
template<typename T, int S>
struct TBitCastBaseTypeIs16Bit<vector<T, S> >
{ 
#if BITCAST_16BIT_SUPPORTED
    static const bool Result = TBitCastTypeIsSame<T, float16_t>::Result
                            || TBitCastTypeIsSame<T,  uint16_t>::Result
                            || TBitCastTypeIsSame<T,   int16_t>::Result;
#else
    static const bool Result = false;
#endif
};
template<typename TDst, typename TSrc>
struct TBitCastIfSupported;

#define BITCAST_SUPPORTED(TDst, TSrc, Intrinsic)                                    \
    template<int S> struct TBitCastIfSupported<vector<TDst, S>, vector<TSrc, S> >   \
    {                                                                               \
        static vector<TDst, S> bit_cast(vector<TSrc, S> x) { return Intrinsic(x); } \
    }

    BITCAST_SUPPORTED(float,  uint, asfloat);
    BITCAST_SUPPORTED(float,   int, asfloat);
    BITCAST_SUPPORTED( uint, float,  asuint);
    BITCAST_SUPPORTED( uint,   int,  asuint);
    BITCAST_SUPPORTED(  int, float,   asint);
    BITCAST_SUPPORTED(  int,  uint,   asint);

    #if BITCAST_16BIT_SUPPORTED
        BITCAST_SUPPORTED(float16_t,  uint16_t, asfloat16);
        BITCAST_SUPPORTED(float16_t,   int16_t, asfloat16);
        BITCAST_SUPPORTED( uint16_t, float16_t,  asuint16);
        BITCAST_SUPPORTED( uint16_t,   int16_t,  asuint16);
        BITCAST_SUPPORTED(  int16_t, float16_t,   asint16);
        BITCAST_SUPPORTED(  int16_t,  uint16_t,   asint16);
    #endif

#undef BITCAST_SUPPORTED

#if BITCAST_16BIT_SUPPORTED && BITCAST_AUX_BASE_TYPE

    uint asuint(uint16_t2 x) { return (uint(x.y) << 16u) | uint(x.x); }
    uint asuint(int16_t2 x) { return asuint(asuint16(x)); }
    uint asuint(float16_t2 x) { return asuint(asuint16(x)); }

    uint16_t2 asuint16(uint x) { return uint16_t2(x & 0xffffu, x >> 16u); };
    int16_t2 asint16(uint x) { return asint16(asuint16(x)); };
    float16_t2 asfloat16(uint x) { return asfloat16(asuint16(x)); };

    #define BITCAST_SUPPORTED(TDst, TSrc, Intrinsic)                                        \
        template<> struct TBitCastIfSupported<vector<TDst, 2>, vector<TSrc, 4> >            \
        {                                                                                   \
            static vector<TDst, 2> bit_cast(vector<TSrc, 4> x)                              \
            {                                                                               \
                return vector<TDst, 2>(Intrinsic(asuint(x.xy)), Intrinsic(asuint(x.zw)));   \
            }                                                                               \
        }

        BITCAST_SUPPORTED(uint,  uint16_t, asuint);
        BITCAST_SUPPORTED(uint,   int16_t, asuint);
        BITCAST_SUPPORTED(uint, float16_t, asuint);

        BITCAST_SUPPORTED( int,  uint16_t, asint);
        BITCAST_SUPPORTED( int,   int16_t, asint);
        BITCAST_SUPPORTED( int, float16_t, asint);

        BITCAST_SUPPORTED(float,  uint16_t, asfloat);
        BITCAST_SUPPORTED(float,   int16_t, asfloat);
        BITCAST_SUPPORTED(float, float16_t, asfloat);

    #undef BITCAST_SUPPORTED

    #define BITCAST_SUPPORTED(TDst, TSrc, Intrinsic)                                    \
        template<> struct TBitCastIfSupported<vector<TDst, 4>, vector<TSrc, 2> >        \
        {                                                                               \
            static vector<TDst, 4> bit_cast(vector<TSrc, 2> x)                          \
            {                                                                           \
                return vector<TDst, 4>(Intrinsic(asuint(x.x)), Intrinsic(asuint(x.y))); \
            }                                                                           \
        }

        BITCAST_SUPPORTED(float16_t,  uint, asfloat16);
        BITCAST_SUPPORTED(float16_t,   int, asfloat16);
        BITCAST_SUPPORTED(float16_t, float, asfloat16);

        BITCAST_SUPPORTED(uint16_t,  uint, asuint16);
        BITCAST_SUPPORTED(uint16_t,   int, asuint16);
        BITCAST_SUPPORTED(uint16_t, float, asuint16);
    
        BITCAST_SUPPORTED(int16_t,  uint, asint16);
        BITCAST_SUPPORTED(int16_t,   int, asint16);
        BITCAST_SUPPORTED(int16_t, float, asint16);

    #undef BITCAST_SUPPORTED

#endif /** BITCAST_16BIT_SUPPORTED && BITCAST_AUX_BASE_TYPE */
)__de___l___im__"
LR"__de___l___im__(
template<typename TDst, 
         typename TSrc, 
         bool IsDstBase16Bit = TBitCastBaseTypeIs16Bit<TDst>::Result, 
         bool IsSrcBase16Bit = TBitCastBaseTypeIs16Bit<TSrc>::Result >
struct TBitCastWidenOrShrink;

template<typename TDst, typename TSrc>
struct TBitCastWidenOrShrinkIfSameBase
{
    static TDst bit_cast(TSrc x) { return TBitCastWidenOrShrink<TDst, TSrc>::bit_cast(x); }
};
template<typename T, int SDst, int SSrc>
struct TBitCastWidenOrShrinkIfSameBase<vector<T, SDst>, vector<T, SSrc> >
{
    static vector<T, SDst> bit_cast(vector<T, SSrc> x)
    {
        vector<T, SDst> o;
        uint i;
        [unroll] for (i = 0; i < min(SDst, SSrc); ++i)
        {
            o[i] = x[i];
        }
        [unroll] for (i = min(SDst, SSrc) - 1; i < SDst; ++i)
        {
            o[i] = 0;
        }
        return o;
    }
};
template<typename TDst, typename TSrc>
struct TBitCastIfNotSame
{
    static TDst bit_cast(TSrc x) { return TBitCastWidenOrShrinkIfSameBase<TDst, TSrc>::bit_cast(x); }
};
template<typename T>
struct TBitCastIfNotSame<T, T>
{
    static T bit_cast(T x) { return x; }
};
template<typename TDst, typename TSrc, int SDst, int SSrc, bool IsBase16BitOr32Bit>
struct TBitCastWidenOrShrink<vector<TDst, SDst>, vector<TSrc, SSrc>, IsBase16BitOr32Bit, IsBase16BitOr32Bit>
{
    static vector<TDst, SDst> bit_cast(vector<TSrc, SSrc> x)
    {
        vector<TSrc, SDst> o;
        uint i;
        [unroll] for (i = 0; i < min(SDst, SSrc); ++i)
        {
            o[i] = x[i];
        }
        [unroll] for (i = min(SDst, SSrc) - 1; i < SDst; ++i)
        {
            o[i] = 0;
        }
        return TBitCastIfSupported< vector<TDst, SDst>, vector<TSrc, SDst> >::bit_cast(o);
    }
};
template<typename TDst, typename TSrc, int SDst, int SSrc>
struct TBitCastWidenOrShrink<vector<TDst, SDst>, vector<TSrc, SSrc>, true, false>
{
    static vector<TDst, SDst> bit_cast(vector<TSrc, SSrc> x)
    {
        vector<TSrc, 2> xaligned = TBitCastIfNotSame<vector<TSrc, 2>, vector<TSrc, SSrc> >::bit_cast(x);
        vector<TDst, 4> oaligned = TBitCastIfSupported<vector<TDst, 4> , vector<TSrc, 2> >::bit_cast(xaligned);
        return TBitCastIfNotSame<vector<TDst, SDst>, vector<TDst, 4> >::bit_cast(oaligned);
    }
};
template<typename TDst, typename TSrc, int SDst, int SSrc>
struct TBitCastWidenOrShrink<vector<TDst, SDst>, vector<TSrc, SSrc>, false, true>
{
    static vector<TDst, SDst> bit_cast(vector<TSrc, SSrc> x)
    {
        /**
         *  NOTE: when the base type of the source is 16-bit, widen to a vector
         *        containing four elements (which is maximal possible for built-in vector
         *        then do the cast to 2-component 32-bit vector and then widen or shrink 
         *        to a vector with actual number of 32-bit components
         */
        vector<TSrc, 4> xaligned = TBitCastIfNotSame<vector<TSrc, 4>, vector<TSrc, SSrc> >::bit_cast(x);
        vector<TDst, 2> oaligned = TBitCastIfSupported<vector<TDst, 2> , vector<TSrc, 4> >::bit_cast(xaligned);
        return TBitCastIfNotSame<vector<TDst, SDst>, vector<TDst, 2> >::bit_cast(oaligned);
    }
};
template<typename TDst, typename TSrc> struct TBitCastVectorizeSrc
{
    static TDst bit_cast(TSrc x) { return TBitCastIfNotSame<TDst, vector<TSrc, 1> >::bit_cast(vector<TSrc, 1>(x)); }
};
template<typename TDst, typename TSrc, int SSrc> struct TBitCastVectorizeSrc<TDst, vector<TSrc, SSrc> >
{
    static TDst bit_cast(vector<TSrc, SSrc> x) { return TBitCastIfNotSame<TDst, vector<TSrc, SSrc> >::bit_cast(x); }
};
template<typename TDst, typename TSrc> struct TBitCastVectorizeDst
{
    static TDst bit_cast(TSrc x) { return TBitCastVectorizeSrc<vector<TDst, 1>, TSrc>::bit_cast(x).x; }
};
template<typename TDst, typename TSrc, int SDst> struct TBitCastVectorizeDst<vector<TDst, SDst>, TSrc>
{
    static vector<TDst, SDst> bit_cast(TSrc x) { return TBitCastVectorizeSrc<vector<TDst, SDst>, TSrc>::bit_cast(x); }
};
template<typename TDstType, typename TSrcType>
static TDstType bit_cast(TSrcType x)
{
    return TBitCastVectorizeDst<TDstType, TSrcType>::bit_cast(x);
}
)__de___l___im__";

} // namespace CppSL::HLSL