#include "container_test_types.hpp"
#include "SkrTestFramework/framework.hpp"

// 包含 std::format 用于对比测试
#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
    #include <format>
    #include <iostream>  // 为 doctest 输出 std::string_view 所需
    #define SKR_HAS_STD_FORMAT 1
    
    // 为 std::u8string 添加 formatter 特化
    template<>
    struct std::formatter<std::u8string> : std::formatter<std::string> {
        template<typename FormatContext>
        auto format(const std::u8string& str, FormatContext& ctx) const {
            return std::formatter<std::string>::format(
                std::string(reinterpret_cast<const char*>(str.data()), str.size()), 
                ctx
            );
        }
    };
    
    // 为 std::u8string_view 添加 formatter 特化
    template<>
    struct std::formatter<std::u8string_view> : std::formatter<std::string_view> {
        template<typename FormatContext>
        auto format(const std::u8string_view& str, FormatContext& ctx) const {
            return std::formatter<std::string_view>::format(
                std::string_view(reinterpret_cast<const char*>(str.data()), str.size()), 
                ctx
            );
        }
    };
    
    // 为 char8_t 数组添加 formatter 特化
    template<size_t N>
    struct std::formatter<char8_t[N]> : std::formatter<std::string_view> {
        template<typename FormatContext>
        auto format(const char8_t (&str)[N], FormatContext& ctx) const {
            return std::formatter<std::string_view>::format(
                std::string_view(reinterpret_cast<const char*>(str), N-1), // N-1 to exclude null terminator
                ctx
            );
        }
    };
    
    // 为 const char8_t* 添加 formatter 特化
    template<>
    struct std::formatter<const char8_t*> : std::formatter<std::string_view> {
        template<typename FormatContext>
        auto format(const char8_t* str, FormatContext& ctx) const {
            return std::formatter<std::string_view>::format(
                std::string_view(reinterpret_cast<const char*>(str)), 
                ctx
            );
        }
    };
    
    // 为 char8_t 添加 formatter 特化
    template<>
    struct std::formatter<char8_t> {
        constexpr auto parse(std::format_parse_context& ctx) {
            return ctx.begin();
        }
        
        template<typename FormatContext>
        auto format(char8_t c, FormatContext& ctx) const {
            return std::format_to(ctx.out(), "{}", static_cast<char>(c));
        }
    };
    
    // 为 SakuraEngine 字符串类型添加 formatter 特化
    template<typename TSize>
    struct std::formatter<skr::container::U8StringView<TSize>> : std::formatter<std::string_view> {
        template<typename FormatContext>
        auto format(const skr::container::U8StringView<TSize>& str, FormatContext& ctx) const {
            return std::formatter<std::string_view>::format(
                std::string_view(reinterpret_cast<const char*>(str.data()), str.size()),
                ctx
            );
        }
    };
    
    template<typename Memory>
    struct std::formatter<skr::container::U8String<Memory>> : std::formatter<std::string_view> {
        template<typename FormatContext>
        auto format(const skr::container::U8String<Memory>& str, FormatContext& ctx) const {
            return std::formatter<std::string_view>::format(
                std::string_view(reinterpret_cast<const char*>(str.data()), str.size()),
                ctx
            );
        }
    };
#else
    #define SKR_HAS_STD_FORMAT 0
#endif

// 验证 std::format 和 skr::format 一致性的辅助函数
template<typename... Args>
void VERIFY_FORMAT_CONSISTENCY(skr::StringView expected, skr::StringView format_spec, Args&&... args)
{
    auto skr_result = skr::format(format_spec, std::forward<Args>(args)...);

#if SKR_HAS_STD_FORMAT
    // 使用 std::vformat 进行运行时格式字符串比较
    std::string format_str(reinterpret_cast<const char*>(format_spec.data()), format_spec.size());
    auto std_result = std::vformat(format_str, std::make_format_args(args...));
    REQUIRE_EQ(std::string_view(std_result), std::string_view((const char*)expected.data(), expected.size()));
        
    std::string skr_str(reinterpret_cast<const char*>(skr_result.data()), skr_result.size());
    if (std_result != skr_str) {
        REQUIRE_EQ(std::string_view(std_result), std::string_view(skr_str));
    }
#endif

    // 测试 skr::format
    REQUIRE_EQ(expected, skr_result);
}

// char8_t 特化版本
void VERIFY_FORMAT_CONSISTENCY(skr::StringView expected, skr::StringView format_spec, char8_t arg)
{
    // 测试 skr::format
    auto skr_result = skr::format(format_spec, arg);
    REQUIRE_EQ(expected, skr_result);
    
#if SKR_HAS_STD_FORMAT
    // 使用 std::vformat 进行运行时格式字符串比较
    std::string format_str(reinterpret_cast<const char*>(format_spec.data()), format_spec.size());
    char converted_arg = static_cast<char>(arg);
    auto std_result = std::vformat(format_str, std::make_format_args(converted_arg));
        
    std::string skr_str(reinterpret_cast<const char*>(skr_result.data()), skr_result.size());
    if (std_result != skr_str) {
        REQUIRE_EQ(std::string_view(std_result), std::string_view(skr_str));
    }
#endif
}

TEST_CASE("test format basics")
{
    using namespace skr::test_container;

    SUBCASE("escaped braces")
    {
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"{}" }, u8"{{}}");
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"{test}" }, u8"{{test}}");
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"}{" }, u8"}}{{");
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"hello {} world" }, u8"hello {{}} world");
    }

    SUBCASE("indexed and auto formatting")
    {
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"1 2 3" }, u8"{} {} {}", 1, 2, 3);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3 1 2" }, u8"{2} {0} {1}", 1, 2, 3);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"1 1 1" }, u8"{0} {0} {0}", 1, 2, 3);
    }

    SUBCASE("boolean")
    {
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"true" }, u8"{}", true);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"false" }, u8"{}", false);
    }

    SUBCASE("char types")
    {
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"A" }, u8"{}", 'A');
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"Z" }, u8"{}", skr_char8('Z'));
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"!" }, u8"{:c}", 33); // ASCII !
    }

    SUBCASE("string types")
    {
        const char* cstr = "hello";
        const skr_char8* u8str = u8"world";
        std::string std_str = "std::string";
        std::u8string std_u8str = u8"std::u8string";
        
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"hello" }, u8"{}", cstr);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"world" }, u8"{}", u8str);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"std::string" }, u8"{}", std_str);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"std::u8string" }, u8"{}", std_u8str);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"view" }, u8"{}", std::string_view("view"));
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"u8view" }, u8"{}", std::u8string_view(u8"u8view"));
        
        // Custom string types - complex multi-arg format
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"繁星明 😀😀" },
                   u8"{}{}明{}{}", u8"繁",
                          StringView{ u8"星" },
                          String{ u8" 😀" },
                          String{ u8"😀" }.c_str());
    }

    SUBCASE("pointer types")
    {
        // Note: std::format may not support nullptr formatting consistently
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"0x0" }, u8"{}", nullptr);
        
        // Pointer formatting depends on pointer size
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"0x75bcd15" }, u8"{}", reinterpret_cast<void*>(123456789));
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"0x0" }, u8"{}", reinterpret_cast<void*>(0));
        
        const void* cptr = reinterpret_cast<const void*>(0xDEADBEEF);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"0xdeadbeef" }, u8"{}", cptr);
        
        // Test high addresses on 64-bit systems
        if (sizeof(void*) == 8)
        {
            const void* high_addr = reinterpret_cast<const void*>(0x7FFFFFFFFFFFFFFF);
            VERIFY_FORMAT_CONSISTENCY(StringView{ u8"0x7fffffffffffffff" }, u8"{}", high_addr);
        }
    }
}

TEST_CASE("test integer format")
{
    using namespace skr::test_container;

    SUBCASE("basic decimal")
    {
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"0" }, u8"{}", 0);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"42" }, u8"{}", 42);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-42" }, u8"{}", -42);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"2147483647" }, u8"{}", INT32_MAX);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-2147483648" }, u8"{}", INT32_MIN);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"18446744073709551615" }, u8"{}", UINT64_MAX);
    }

    SUBCASE("type specifiers")
    {
        // Decimal
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"255" }, u8"{:d}", 255);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-255" }, u8"{:d}", -255);
        
        // Binary - Note: std::format may not support binary format
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"11111111" }, u8"{:b}", 255);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"0b11111111" }, u8"{:#b}", 255);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"101010" }, u8"{:b}", 42);
        
        // Octal
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"377" }, u8"{:o}", 255);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"52" }, u8"{:o}", 42);
        
        // Hexadecimal
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"ff" }, u8"{:x}", 255);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"FF" }, u8"{:X}", 255);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"0xff" }, u8"{:#x}", 255);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"0XFF" }, u8"{:#X}", 255);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"deadbeef" }, u8"{:x}", 0xDEADBEEF);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"DEADBEEF" }, u8"{:X}", 0xDEADBEEF);
        
        // Character
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"A" }, u8"{:c}", 65);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"z" }, u8"{:c}", 122);
        // Note: Out-of-range character behavior may differ between implementations
        // std::format throws on out-of-range values, so test separately
        REQUIRE_EQ(StringView{ u8"?" }, skr::format(u8"{:c}", 200)); // Out of ASCII range
    }

    SUBCASE("width and padding")
    {
        // Zero padding
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"00042" }, u8"{:05}", 42);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"00255" }, u8"{:05d}", 255);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-0042" }, u8"{:05}", -42);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"0x00ff" }, u8"{:#06x}", 255);
        
        // Space padding
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"   42" }, u8"{:5}", 42);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"  -42" }, u8"{:5}", -42);
        
        // Custom fill character
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"***42" }, u8"{:*>5}", 42);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"42***" }, u8"{:*<5}", 42);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"__-42" }, u8"{:_>5}", -42);
    }

    SUBCASE("alignment")
    {
        // Left align
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"42   " }, u8"{:<5}", 42);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-42  " }, u8"{:<5}", -42);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"ff   " }, u8"{:<5x}", 255);
        
        // Right align (default)
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"   42" }, u8"{:>5}", 42);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"  -42" }, u8"{:>5}", -42);
        
        // Center align
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8" 42  " }, u8"{:^5}", 42);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8" -42 " }, u8"{:^5}", -42);
    }

    SUBCASE("sign handling")
    {
        // Plus sign
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"+42" }, u8"{:+}", 42);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-42" }, u8"{:+}", -42);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"+0" }, u8"{:+}", 0);
        
        // Space for positive
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8" 42" }, u8"{: }", 42);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-42" }, u8"{: }", -42);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8" 0" }, u8"{: }", 0);
        
        // Combined with width
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"  +42" }, u8"{:+5}", 42);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"+0042" }, u8"{:+05}", 42);
    }

    SUBCASE("thousands separator")
    {
        // Note: std::format doesn't support thousands separator, test skr::format only
        REQUIRE_EQ(StringView{ u8"1,234" }, skr::format(u8"{:'}", 1234));
        REQUIRE_EQ(StringView{ u8"1,234,567" }, skr::format(u8"{:'}", 1234567));
        REQUIRE_EQ(StringView{ u8"-1,234,567" }, skr::format(u8"{:'}", -1234567));
        REQUIRE_EQ(StringView{ u8"123" }, skr::format(u8"{:'}", 123)); // No separator for < 1000
    }

    SUBCASE("combined format specifiers")
    {
        // Width + type + prefix
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"  0xff" }, u8"{:#6x}", 255);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"0x00ff" }, u8"{:#06x}", 255);
        
        // All together
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"+00042" }, u8"{:+06d}", 42);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"0x00000000deadbeef" }, u8"{:#018x}", 0xDEADBEEF);
        // Note: thousands separator is not standard in std::format, test skr::format only
        // Debug: test basic thousands separator first
        REQUIRE_EQ(StringView{ u8"1,234" }, skr::format(u8"{:'}", 1234));
        // Debug: test thousands separator with different combinations
        REQUIRE_EQ(StringView{ u8"+1,234" }, skr::format(u8"{:+'}", 1234));
        REQUIRE_EQ(StringView{ u8"1,234      " }, skr::format(u8"{:<11'}", 1234));
        REQUIRE_EQ(StringView{ u8"+1,234     " }, skr::format(u8"{:+<11'}", 1234));
    }

    SUBCASE("edge cases")
    {
        // Zero with various bases
        // Note: binary format may not be in std::format
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"0" }, u8"{:b}", 0);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"0" }, u8"{:o}", 0);
        // Note: updated behavior - hex zero now shows prefix
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"0x0" }, u8"{:#x}", 0);
        
        // Negative numbers with unsigned format
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-ff" }, u8"{:x}", -255);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-0xff" }, u8"{:#x}", -255);
        
        // Various integer types
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"255" }, u8"{}", uint8_t(255));
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-128" }, u8"{}", int8_t(-128));
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"65535" }, u8"{}", uint16_t(65535));
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-32768" }, u8"{}", int16_t(-32768));
    }
}

TEST_CASE("test float format")
{
    using namespace skr::test_container;

    SUBCASE("basic floats")
    {
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"0" }, u8"{}", 0.0f);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"1" }, u8"{}", 1.0f);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-1" }, u8"{}", -1.0f);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.14" }, u8"{}", 3.14);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-3.14" }, u8"{}", -3.14);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"60.004" }, u8"{}", 60.004);
    }

    SUBCASE("special values")
    {
        // Note: Special value formatting may vary between implementations
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"inf" }, u8"{}", std::numeric_limits<float>::infinity());
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-inf" }, u8"{}", -std::numeric_limits<float>::infinity());
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"nan" }, u8"{}", std::numeric_limits<float>::quiet_NaN());
        
        // Uppercase versions
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"INF" }, u8"{:F}", std::numeric_limits<float>::infinity());
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-INF" }, u8"{:F}", -std::numeric_limits<float>::infinity());
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"NAN" }, u8"{:F}", std::numeric_limits<float>::quiet_NaN());
    }

    SUBCASE("fixed format")
    {
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.14" }, u8"{:.2f}", 3.14159);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.1" }, u8"{:.1f}", 3.14159);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3" }, u8"{:.0f}", 3.14159);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.141590" }, u8"{:.6f}", 3.14159);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-3.14000" }, u8"{:.5f}", -3.14);
        
        // Uppercase F
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.14" }, u8"{:.2F}", 3.14159);
        
        // Rounding
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.15" }, u8"{:.2f}", 3.145);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.14" }, u8"{:.2f}", 3.144);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"10.00" }, u8"{:.2f}", 9.999);
    }

    SUBCASE("scientific format")
    {
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.14e+00" }, u8"{:.2e}", 3.14);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.14E+00" }, u8"{:.2E}", 3.14);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"1.23e+02" }, u8"{:.2e}", 123.0);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"1.23e-02" }, u8"{:.2e}", 0.0123);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-1.23e+02" }, u8"{:.2e}", -123.0);
        
        // Very large and small numbers
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"1.00e+10" }, u8"{:.2e}", 1e10);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"1.00e-10" }, u8"{:.2e}", 1e-10);
    }

    SUBCASE("general format")
    {
        // g format chooses between fixed and scientific
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.14" }, u8"{:.3g}", 3.14);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.14" }, u8"{:.3G}", 3.14);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"1.23e+06" }, u8"{:.3g}", 1234567.0);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"0.00123" }, u8"{:.3g}", 0.00123);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"1.23e-05" }, u8"{:.3g}", 0.0000123);
        
        // Precision 0 special case
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3" }, u8"{:.0g}", 3.14);
    }

    SUBCASE("hexadecimal format")
    {
        // Note: C++20 std::format hexadecimal float format does not include 0x prefix by default
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"1.91eb851eb851fp+1" }, u8"{:a}", 3.14);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"1.91EB851EB851FP+1" }, u8"{:A}", 3.14);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"1p+0" }, u8"{:a}", 1.0);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"0p+0" }, u8"{:a}", 0.0);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-1p+0" }, u8"{:a}", -1.0);
    }

    SUBCASE("percent format")
    {
        // Note: Percent format is not standard in C++20 std::format, test skr::format only
        REQUIRE_EQ(StringView{ u8"50.00%" }, skr::format(u8"{:.2%}", 0.5));
        REQUIRE_EQ(StringView{ u8"100.00%" }, skr::format(u8"{:.2%}", 1.0));
        REQUIRE_EQ(StringView{ u8"123.46%" }, skr::format(u8"{:.2%}", 1.23456));
        REQUIRE_EQ(StringView{ u8"-50.00%" }, skr::format(u8"{:.2%}", -0.5));
    }

    SUBCASE("width and alignment")
    {
        // Right align (default)
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"   3.14" }, u8"{:7.2f}", 3.14);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"  -3.14" }, u8"{:7.2f}", -3.14);
        
        // Left align
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.14   " }, u8"{:<7.2f}", 3.14);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-3.14  " }, u8"{:<7.2f}", -3.14);
        
        // Center align
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8" 3.14  " }, u8"{:^7.2f}", 3.14);
        
        // Zero padding
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"0003.14" }, u8"{:07.2f}", 3.14);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-003.14" }, u8"{:07.2f}", -3.14);
        
        // Custom fill
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"***3.14" }, u8"{:*>7.2f}", 3.14);
    }

    SUBCASE("sign handling")
    {
        // Plus sign
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"+3.14" }, u8"{:+.2f}", 3.14);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-3.14" }, u8"{:+.2f}", -3.14);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"+0.00" }, u8"{:+.2f}", 0.0);
        
        // Space for positive
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8" 3.14" }, u8"{: .2f}", 3.14);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"-3.14" }, u8"{: .2f}", -3.14);
    }

    SUBCASE("alternate form (#)")
    {
        // Forces decimal point
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3." }, u8"{:#.0f}", 3.0);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.00" }, u8"{:#.2f}", 3.0);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.e+00" }, u8"{:#.0e}", 3.0);
        
        // g format removes trailing zeros unless # is used
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3" }, u8"{:.2g}", 3.0);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.0" }, u8"{:#.2g}", 3.0);
    }

    SUBCASE("precision edge cases")
    {
        // Default precision
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.141593" }, u8"{:f}", 3.14159265);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.141593e+00" }, u8"{:e}", 3.14159265);
        
        // Very high precision
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.1415926500000002" }, u8"{:.16f}", 3.14159265);
        
        // Zero precision
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3" }, u8"{:.0f}", 3.14159);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3e+00" }, u8"{:.0e}", 3.14159);
    }

    SUBCASE("float vs double")
    {
        float f = 3.14159265f;
        double d = 3.14159265;
        
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.141593" }, u8"{:.6f}", f);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.141593" }, u8"{:.6f}", d);
        
        // Precision differences
        float f_small = 1.23456789e-30f;
        double d_small = 1.23456789e-30;
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"1.23e-30" }, u8"{:.2e}", f_small);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"1.23e-30" }, u8"{:.2e}", d_small);
    }

    SUBCASE("combined specifiers")
    {
        // All options together
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"***3.14****" }, u8"{:*^11.2f}", 3.14);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"+003.140" }, u8"{:+08.3f}", 3.14);
        VERIFY_FORMAT_CONSISTENCY(StringView{ u8"3.14e+00   " }, u8"{: <11.2e}", 3.14);
    }
}