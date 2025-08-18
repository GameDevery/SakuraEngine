#include <SkrBase/containers/string/format.hpp>
#include <SkrBase/containers/string/string.hpp>
#include <cstring>
#include <algorithm>
#include <charconv>

namespace skr::container
{

// 内部实现细节
namespace detail
{
    // 整数格式化标志
    struct IntegerFormatFlags 
    {
        enum class Type {
            Decimal,     // d - 十进制
            Binary,      // b - 二进制
            Octal,       // o - 八进制
            Hex,         // x/X - 十六进制
            Character    // c - 字符
        };
        
        Type type = Type::Decimal;
        int width = 0;                   // 最小宽度
        bool uppercase = false;          // 大写形式 (X)
        bool show_prefix = false;        // 显示前缀 (#)
        bool show_positive = false;      // 显示正号 (+)
        bool left_align = false;         // 左对齐 (-)
        bool center_align = false;       // 居中对齐 (^)
        bool zero_pad = false;           // 零填充 (0)
        bool space_positive = false;     // 正数前加空格 ( )
        bool thousands_sep = false;      // 千位分隔符 (')
        skr_char8 fill_char = u8' ';     // 填充字符
    };
    
    // 解析格式说明符
    IntegerFormatFlags parse_integer_spec(const skr_char8* spec_data, size_t spec_size)
    {
        IntegerFormatFlags flags;
        
        if (spec_size == 0)
            return flags;
            
        size_t parsing_start = 0;
        size_t parsing_end = spec_size;
        
        // 解析类型说明符 (最后一个字符)
        if (parsing_end > parsing_start)
        {
            auto last_ch = spec_data[parsing_end - 1];
            switch (last_ch)
            {
                case u8'd':
                    flags.type = IntegerFormatFlags::Type::Decimal;
                    parsing_end--;
                    break;
                    
                case u8'b':
                    flags.type = IntegerFormatFlags::Type::Binary;
                    parsing_end--;
                    break;
                    
                case u8'o':
                    flags.type = IntegerFormatFlags::Type::Octal;
                    parsing_end--;
                    break;
                    
                case u8'x':
                    flags.type = IntegerFormatFlags::Type::Hex;
                    flags.uppercase = false;
                    parsing_end--;
                    break;
                    
                case u8'X':
                    flags.type = IntegerFormatFlags::Type::Hex;
                    flags.uppercase = true;
                    parsing_end--;
                    break;
                    
                case u8'c':
                    flags.type = IntegerFormatFlags::Type::Character;
                    parsing_end--;
                    break;
            }
        }
        
        size_t i = parsing_start;
        
        // 1. 首先检查是否有填充字符和对齐方式
        if (i < parsing_end)
        {
            // 检查是否有填充字符（下一个字符是对齐符号）
            if (i + 1 < parsing_end)
            {
                auto curr_ch = spec_data[i];
                auto next_ch = spec_data[i + 1];
                if (next_ch == u8'<' || next_ch == u8'>' || next_ch == u8'^')
                {
                    // 如果当前字符不是符号标志，则作为填充字符
                    if (curr_ch != u8'+' && curr_ch != u8'-' && curr_ch != u8' ')
                    {
                        flags.fill_char = curr_ch;
                        i++;
                    }
                }
            }
            
            // 检查对齐说明符
            if (i < parsing_end)
            {
                auto ch = spec_data[i];
                if (ch == u8'<')
                {
                    flags.left_align = true;
                    i++;
                }
                else if (ch == u8'>')
                {
                    i++; // 右对齐是默认的，但需要跳过字符
                }
                else if (ch == u8'^')
                {
                    flags.center_align = true;
                    i++;
                }
            }
        }
        
        // 2. 解析符号
        if (i < parsing_end)
        {
            auto ch = spec_data[i];
            if (ch == u8'+')
            {
                flags.show_positive = true;
                i++;
            }
            else if (ch == u8'-' && !flags.left_align) // 只有在没有设置左对齐时才将-作为符号处理
            {
                flags.left_align = true;
                i++;
            }
            else if (ch == u8' ')
            {
                flags.space_positive = true;
                i++;
            }
        }
        
        // 2.5. 再次检查对齐方式（处理 "+<" 这种情况）
        if (i < parsing_end)
        {
            auto ch = spec_data[i];
            if (ch == u8'<' && !flags.left_align && !flags.center_align)
            {
                flags.left_align = true;
                i++;
            }
            else if (ch == u8'>' && !flags.left_align && !flags.center_align)
            {
                i++; // 右对齐是默认的，但需要跳过字符
            }
            else if (ch == u8'^' && !flags.left_align && !flags.center_align)
            {
                flags.center_align = true;
                i++;
            }
        }
        
        // 3. 解析替代形式标志
        if (i < parsing_end && spec_data[i] == u8'#')
        {
            flags.show_prefix = true;
            i++;
        }
        
        // 4. 解析零填充
        if (i < parsing_end && spec_data[i] == u8'0')
        {
            flags.zero_pad = true;
            i++;
        }
        
        // 5. 解析宽度
        if (i < parsing_end && spec_data[i] >= u8'0' && spec_data[i] <= u8'9')
        {
            size_t width_end = i;
            while (width_end < parsing_end && 
                   spec_data[width_end] >= u8'0' && 
                   spec_data[width_end] <= u8'9')
            {
                width_end++;
            }
            
            const auto [last, err] = std::from_chars(
                (const char*)(spec_data + i),
                (const char*)(spec_data + width_end),
                flags.width
            );
            SKR_VERIFY(err == std::errc() && "Invalid width in format specification!");
            i = width_end;
        }
        
        // 6. 解析千位分隔符
        if (i < parsing_end && spec_data[i] == u8'\'')
        {
            flags.thousands_sep = true;
            i++;
        }
        
        // 左对齐或居中对齐时不能零填充
        if (flags.left_align || flags.center_align)
            flags.zero_pad = false;
            
        return flags;
    }
    
    // 获取基数
    constexpr int get_base(IntegerFormatFlags::Type type)
    {
        switch (type)
        {
            case IntegerFormatFlags::Type::Binary: return 2;
            case IntegerFormatFlags::Type::Octal: return 8;
            case IntegerFormatFlags::Type::Hex: return 16;
            default: return 10;
        }
    }
    
    // 获取前缀
    const skr_char8* get_prefix(IntegerFormatFlags::Type type, bool uppercase)
    {
        switch (type)
        {
            case IntegerFormatFlags::Type::Binary: return u8"0b";
            case IntegerFormatFlags::Type::Octal: return u8"0o";
            case IntegerFormatFlags::Type::Hex: return uppercase ? u8"0X" : u8"0x";
            default: return u8"";
        }
    }
    
    // 转换单个数字到字符
    constexpr skr_char8 digit_to_char(int digit, bool uppercase)
    {
        if (digit < 10)
            return u8'0' + digit;
        return (uppercase ? u8'A' : u8'a') + (digit - 10);
    }
    
    // 格式化无符号整数的内部实现
    void format_unsigned_impl(const StringFunctionTable& table, uint64_t value, const IntegerFormatFlags& flags)
    {
        // 处理字符类型
        if (flags.type == IntegerFormatFlags::Type::Character)
        {
            if (value > 127) // 只支持 ASCII 字符
            {
                table.append_u8str(table.string_ptr, u8"?", 1);
            }
            else
            {
                table.append_char(table.string_ptr, static_cast<skr_char8>(value));
            }
            return;
        }
        
        // 准备缓冲区
        constexpr size_t buffer_size = 128;
        char buffer[buffer_size];
        size_t length = 0;
        
        // 获取基数
        const int base = get_base(flags.type);
        
        // 转换为字符串
        if (value == 0)
        {
            buffer[length++] = '0';
        }
        else
        {
            uint64_t temp = value;
            size_t start = length;
            
            while (temp > 0)
            {
                int digit = temp % base;
                buffer[length++] = digit_to_char(digit, flags.uppercase);
                temp /= base;
            }
            
            // 反转数字部分
            std::reverse(buffer + start, buffer + length);
        }
        
        // 添加千位分隔符（仅十进制）
        size_t digits_length = length;
        if (flags.thousands_sep && flags.type == IntegerFormatFlags::Type::Decimal && length >= 4)
        {
            // 创建带分隔符的新缓冲区
            char sep_buffer[buffer_size];
            size_t sep_length = 0;
            size_t digit_count = 0;
            
            for (int i = length - 1; i >= 0; i--)
            {
                if (digit_count > 0 && digit_count % 3 == 0)
                {
                    sep_buffer[sep_length++] = ',';
                }
                sep_buffer[sep_length++] = buffer[i];
                digit_count++;
            }
            
            // 反转并复制回原缓冲区
            length = sep_length;
            for (size_t i = 0; i < length; i++)
            {
                buffer[i] = sep_buffer[length - 1 - i];
            }
        }
        
        // 准备前缀
        const skr_char8* prefix = u8"";
        size_t prefix_length = 0;
        
        if (flags.show_prefix)
        {
            // 对于十六进制，即使值为 0 也显示前缀（与 C++ std::format 行为一致）
            if (flags.type == IntegerFormatFlags::Type::Hex || value != 0)
            {
                prefix = get_prefix(flags.type, flags.uppercase);
                prefix_length = std::strlen((const char*)prefix);
            }
        }
        
        // 计算总长度和填充
        size_t total_length = length + prefix_length;
        size_t padding = 0;
        
        if (flags.width > 0 && total_length < static_cast<size_t>(flags.width))
        {
            padding = flags.width - total_length;
        }
        
        // 输出格式化的结果
        if (flags.left_align)
        {
            // 左对齐
            if (prefix_length > 0)
                table.append_u8str(table.string_ptr, prefix, prefix_length);
            table.append_cstr(table.string_ptr, buffer, length);
            table.add_chars(table.string_ptr, flags.fill_char, padding);
        }
        else if (flags.center_align)
        {
            // 居中对齐
            size_t left_padding = padding / 2;
            size_t right_padding = padding - left_padding;
            
            table.add_chars(table.string_ptr, flags.fill_char, left_padding);
            if (prefix_length > 0)
                table.append_u8str(table.string_ptr, prefix, prefix_length);
            table.append_cstr(table.string_ptr, buffer, length);
            table.add_chars(table.string_ptr, flags.fill_char, right_padding);
        }
        else if (flags.zero_pad && !flags.left_align && !flags.center_align)
        {
            // 零填充（前缀在前）
            if (prefix_length > 0)
                table.append_u8str(table.string_ptr, prefix, prefix_length);
            table.add_chars(table.string_ptr, u8'0', padding);
            table.append_cstr(table.string_ptr, buffer, length);
        }
        else
        {
            // 右对齐（默认）
            table.add_chars(table.string_ptr, flags.fill_char, padding);
            if (prefix_length > 0)
                table.append_u8str(table.string_ptr, prefix, prefix_length);
            table.append_cstr(table.string_ptr, buffer, length);
        }
    }
    
    // 格式化有符号整数的内部实现
    void format_signed_impl(const StringFunctionTable& table, int64_t value, const IntegerFormatFlags& flags)
    {
        // 处理字符类型
        if (flags.type == IntegerFormatFlags::Type::Character)
        {
            if (value < 0 || value > 127) // 只支持 ASCII 字符
            {
                table.append_u8str(table.string_ptr, u8"?", 1);
            }
            else
            {
                table.append_char(table.string_ptr, static_cast<skr_char8>(value));
            }
            return;
        }
        
        // 准备缓冲区
        constexpr size_t buffer_size = 128;
        char buffer[buffer_size];
        size_t length = 0;
        
        // 确定符号
        bool is_negative = value < 0;
        uint64_t abs_value = is_negative ? static_cast<uint64_t>(-value) : static_cast<uint64_t>(value);
        
        // 获取基数
        const int base = get_base(flags.type);
        
        // 转换为字符串
        if (abs_value == 0)
        {
            buffer[length++] = '0';
        }
        else
        {
            uint64_t temp = abs_value;
            size_t start = length;
            
            while (temp > 0)
            {
                int digit = temp % base;
                buffer[length++] = digit_to_char(digit, flags.uppercase);
                temp /= base;
            }
            
            // 反转数字部分
            std::reverse(buffer + start, buffer + length);
        }
        
        // 添加千位分隔符（仅十进制）
        if (flags.thousands_sep && flags.type == IntegerFormatFlags::Type::Decimal && length >= 4)
        {
            // 创建带分隔符的新缓冲区
            char sep_buffer[buffer_size];
            size_t sep_length = 0;
            size_t digit_count = 0;
            
            for (int i = length - 1; i >= 0; i--)
            {
                if (digit_count > 0 && digit_count % 3 == 0)
                {
                    sep_buffer[sep_length++] = ',';
                }
                sep_buffer[sep_length++] = buffer[i];
                digit_count++;
            }
            
            // 反转并复制回原缓冲区
            length = sep_length;
            for (size_t i = 0; i < length; i++)
            {
                buffer[i] = sep_buffer[length - 1 - i];
            }
        }
        
        // 准备符号
        const skr_char8* sign_str = nullptr;
        size_t sign_length = 0;
        
        if (is_negative)
        {
            sign_str = u8"-";
            sign_length = 1;
        }
        else if (flags.show_positive)
        {
            sign_str = u8"+";
            sign_length = 1;
        }
        else if (flags.space_positive)
        {
            sign_str = u8" ";
            sign_length = 1;
        }
        
        // 准备前缀
        const skr_char8* prefix = u8"";
        size_t prefix_length = 0;
        
        if (flags.show_prefix)
        {
            // 对于十六进制，即使值为 0 也显示前缀（与 C++ std::format 行为一致）
            if (flags.type == IntegerFormatFlags::Type::Hex || abs_value != 0)
            {
                prefix = get_prefix(flags.type, flags.uppercase);
                prefix_length = std::strlen((const char*)prefix);
            }
        }
        
        // 计算总长度和填充
        size_t total_length = length + sign_length + prefix_length;
        size_t padding = 0;
        
        if (flags.width > 0 && total_length < static_cast<size_t>(flags.width))
        {
            padding = flags.width - total_length;
        }
        
        // 输出格式化的结果
        if (flags.left_align)
        {
            // 左对齐
            if (sign_length > 0)
                table.append_u8str(table.string_ptr, sign_str, sign_length);
            if (prefix_length > 0)
                table.append_u8str(table.string_ptr, prefix, prefix_length);
            table.append_cstr(table.string_ptr, buffer, length);
            table.add_chars(table.string_ptr, flags.fill_char, padding);
        }
        else if (flags.center_align)
        {
            // 居中对齐
            size_t left_padding = padding / 2;
            size_t right_padding = padding - left_padding;
            
            table.add_chars(table.string_ptr, flags.fill_char, left_padding);
            if (sign_length > 0)
                table.append_u8str(table.string_ptr, sign_str, sign_length);
            if (prefix_length > 0)
                table.append_u8str(table.string_ptr, prefix, prefix_length);
            table.append_cstr(table.string_ptr, buffer, length);
            table.add_chars(table.string_ptr, flags.fill_char, right_padding);
        }
        else if (flags.zero_pad && !flags.left_align && !flags.center_align)
        {
            // 零填充（符号和前缀在前）
            if (sign_length > 0)
                table.append_u8str(table.string_ptr, sign_str, sign_length);
            if (prefix_length > 0)
                table.append_u8str(table.string_ptr, prefix, prefix_length);
            table.add_chars(table.string_ptr, u8'0', padding);
            table.append_cstr(table.string_ptr, buffer, length);
        }
        else
        {
            // 右对齐（默认）
            table.add_chars(table.string_ptr, flags.fill_char, padding);
            if (sign_length > 0)
                table.append_u8str(table.string_ptr, sign_str, sign_length);
            if (prefix_length > 0)
                table.append_u8str(table.string_ptr, prefix, prefix_length);
            table.append_cstr(table.string_ptr, buffer, length);
        }
    }
}

// 使用 StringFunctionTable 的类型擦除版本
void format_integer_erased(const StringFunctionTable& table, uint64_t value, const skr_char8* spec_data, size_t spec_size, bool is_signed)
{
    auto flags = detail::parse_integer_spec(spec_data, spec_size);
    detail::format_unsigned_impl(table, value, flags);
}

void format_integer_erased(const StringFunctionTable& table, int64_t value, const skr_char8* spec_data, size_t spec_size)
{
    auto flags = detail::parse_integer_spec(spec_data, spec_size);
    detail::format_signed_impl(table, value, flags);
}

} // namespace skr::container