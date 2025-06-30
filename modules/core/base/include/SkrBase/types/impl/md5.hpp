#pragma once
#include "SkrBase/config.h"
#include "SkrBase/misc/hash.hpp"
#include <compare>

namespace skr
{
struct MD5 {
    union
    {
        // digest based
        uint8_t digest[128 / 8];

        // view based
        struct
        {
            uint32_t a;
            uint32_t b;
            uint32_t c;
            uint32_t d;
        };
    };

    // ctor
    inline MD5()
        : a(0)
        , b(0)
        , c(0)
        , d(0)
    {
    }

    // factory
    static MD5 Parse(const char8_t* str);
    static MD5 Make(const char8_t* str, uint32_t str_size);

    // compare
    inline friend bool operator==(const MD5& lhs, const MD5& rhs)
    {
        return lhs.a == rhs.a &&
               lhs.b == rhs.b &&
               lhs.c == rhs.c &&
               lhs.d == rhs.d;
    }
    inline friend bool operator!=(const MD5& lhs, const MD5& rhs)
    {
        return !(lhs == rhs);
    }
    inline friend bool operator<(const MD5& lhs, const MD5& rhs)
    {
        if (lhs.a != rhs.a) return lhs.a < rhs.a;
        if (lhs.b != rhs.b) return lhs.b < rhs.b;
        if (lhs.c != rhs.c) return lhs.c < rhs.c;
        return lhs.d < rhs.d;
    }
    inline friend bool operator>(const MD5& lhs, const MD5& rhs)
    {
        if (lhs.a != rhs.a) return lhs.a > rhs.a;
        if (lhs.b != rhs.b) return lhs.b > rhs.b;
        if (lhs.c != rhs.c) return lhs.c > rhs.c;
        return lhs.d > rhs.d;
    }
    inline friend bool operator<=(const MD5& lhs, const MD5& rhs)
    {
        return !(lhs > rhs);
    }
    inline friend bool operator>=(const MD5& lhs, const MD5& rhs)
    {
        return !(lhs < rhs);
    }

    // hash
    inline constexpr size_t get_hash() const
    {
        using namespace skr;
        constexpr Hash<uint64_t> hasher{};

        size_t result = hasher.operator()(static_cast<uint64_t>(a) << 32 | b);
        return hash_combine(
            result,
            hasher.operator()(static_cast<uint64_t>(c) << 32 | d)
        );
    }

    // skr hash
    inline static skr_hash _skr_hash(const MD5& md5)
    {
        return md5.get_hash();
    }
};
} // namespace skr