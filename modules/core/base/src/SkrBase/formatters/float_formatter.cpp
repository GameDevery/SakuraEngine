#include <SkrBase/containers/string/format.hpp>
#include <SkrBase/containers/string/string.hpp>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <limits>

namespace skr::container
{

// 内部实现细节
namespace detail
{
    // 浮点数格式化标志
    struct FloatFormatFlags 
    {
        enum class Style {
            Default,    // 默认格式
            Fixed,      // 固定小数位数 (f)
            Scientific, // 科学计数法 (e/E)
            General,    // 通用格式 (g/G)
            Hex,        // 十六进制格式 (a/A)
            Percent     // 百分比格式 (%)
        };
        
        Style style = Style::Default;
        int precision = -1;              // 精度 (.数字)
        int width = 0;                   // 最小宽度
        bool uppercase = false;          // 大写形式 (E/G/A)
        bool show_point = false;         // 总是显示小数点 (#)
        bool show_positive = false;      // 显示正号 (+)
        bool left_align = false;         // 左对齐 (-)
        bool center_align = false;       // 居中对齐 (^)
        bool zero_pad = false;           // 零填充 (0)
        bool space_positive = false;     // 正数前加空格 ( )
        skr_char8 fill_char = u8' ';     // 填充字符
    };
    
    // 解析格式说明符 - 使用原始指针和大小
    FloatFormatFlags parse_float_spec(const skr_char8* spec_data, size_t spec_size)
    {
        FloatFormatFlags flags;
        
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
                case u8'f':
                case u8'F':
                    flags.style = FloatFormatFlags::Style::Fixed;
                    flags.uppercase = (last_ch == u8'F');
                    parsing_end--;
                    break;
                    
                case u8'e':
                case u8'E':
                    flags.style = FloatFormatFlags::Style::Scientific;
                    flags.uppercase = (last_ch == u8'E');
                    parsing_end--;
                    break;
                    
                case u8'g':
                case u8'G':
                    flags.style = FloatFormatFlags::Style::General;
                    flags.uppercase = (last_ch == u8'G');
                    parsing_end--;
                    break;
                    
                case u8'a':
                case u8'A':
                    flags.style = FloatFormatFlags::Style::Hex;
                    flags.uppercase = (last_ch == u8'A');
                    parsing_end--;
                    break;
                    
                case u8'%':
                    flags.style = FloatFormatFlags::Style::Percent;
                    parsing_end--;
                    break;
            }
        }
        
        // 解析精度
        size_t dot_pos = parsing_end;
        for (size_t i = parsing_start; i < parsing_end; ++i)
        {
            if (spec_data[i] == u8'.')
            {
                dot_pos = i;
                break;
            }
        }
        
        if (dot_pos < parsing_end)
        {
            size_t precision_start = dot_pos + 1;
            size_t precision_end = precision_start;
            
            // 找到精度数字的结束位置
            while (precision_end < parsing_end && 
                   spec_data[precision_end] >= u8'0' && 
                   spec_data[precision_end] <= u8'9')
            {
                precision_end++;
            }
            
            if (precision_end > precision_start)
            {
                const auto [last, err] = std::from_chars(
                    (const char*)(spec_data + precision_start),
                    (const char*)(spec_data + precision_end),
                    flags.precision
                );
                SKR_VERIFY(err == std::errc() && "Invalid precision in format specification!");
            }
            else
            {
                // .后面没有数字，精度为0
                flags.precision = 0;
            }
            
            parsing_end = dot_pos;
        }
        
        // 解析宽度和对齐
        size_t i = parsing_start;
        
        // 解析填充字符和对齐方式
        if (i < parsing_end)
        {
            // 可能有填充字符
            if (i + 1 < parsing_end)
            {
                auto next_ch = spec_data[i + 1];
                if (next_ch == u8'<' || next_ch == u8'>' || next_ch == u8'^')
                {
                    flags.fill_char = spec_data[i];
                    i++;
                }
            }
            
            // 检查单独的对齐说明符
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
                    // 右对齐是默认的
                    i++;
                }
                else if (ch == u8'^')
                {
                    flags.center_align = true;
                    i++;
                }
            }
        }
        
        // 解析标志
        while (i < parsing_end)
        {
            auto ch = spec_data[i];
            bool is_flag = true;
            
            switch (ch)
            {
                case u8'#':
                    flags.show_point = true;
                    break;
                case u8'+':
                    flags.show_positive = true;
                    break;
                case u8'-':
                    flags.left_align = true;
                    break;
                case u8' ':
                    flags.space_positive = true;
                    break;
                case u8'0':
                    flags.zero_pad = true;
                    break;
                default:
                    is_flag = false;
                    break;
            }
            
            if (!is_flag)
                break;
                
            i++;
        }
        
        // 解析宽度
        if (i < parsing_end)
        {
            size_t width_end = i;
            while (width_end < parsing_end && 
                   spec_data[width_end] >= u8'0' && 
                   spec_data[width_end] <= u8'9')
            {
                width_end++;
            }
            
            if (width_end > i)
            {
                const auto [last, err] = std::from_chars(
                    (const char*)(spec_data + i),
                    (const char*)(spec_data + width_end),
                    flags.width
                );
                SKR_VERIFY(err == std::errc() && "Invalid width in format specification!");
            }
        }
        
        // 左对齐或居中对齐时不能零填充
        if (flags.left_align || flags.center_align)
            flags.zero_pad = false;
            
        return flags;
    }
    
    // 格式化浮点数的内部实现 - 使用 StringFunctionTable
    void format_float_impl(const StringFunctionTable& table, double value, const FloatFormatFlags& flags)
    {
        // 处理特殊值
        if (std::isnan(value))
        {
            const skr_char8* nan_str = flags.uppercase ? u8"NAN" : u8"nan";
            table.append_u8str(table.string_ptr, nan_str, 3);
            return;
        }
        
        if (std::isinf(value))
        {
            if (value < 0)
                table.append_u8str(table.string_ptr, u8"-", 1);
            else if (flags.show_positive)
                table.append_u8str(table.string_ptr, u8"+", 1);
            else if (flags.space_positive)
                table.append_u8str(table.string_ptr, u8" ", 1);
                
            const skr_char8* inf_str = flags.uppercase ? u8"INF" : u8"inf";
            table.append_u8str(table.string_ptr, inf_str, 3);
            return;
        }
        
        // 百分比格式：先乘以100
        if (flags.style == FloatFormatFlags::Style::Percent)
        {
            value *= 100.0;
        }
        
        // 准备缓冲区
        constexpr size_t buffer_size = 512;
        char buffer[buffer_size];
        size_t length = 0;
        
        // 根据不同的格式选择不同的转换方法
        std::to_chars_result result;
        
        // 确定使用的精度
        int precision = flags.precision;
        if (precision < 0)
        {
            switch (flags.style)
            {
                case FloatFormatFlags::Style::Fixed:
                case FloatFormatFlags::Style::Percent:
                    precision = 6;
                    break;
                case FloatFormatFlags::Style::Scientific:
                    precision = 6;
                    break;
                case FloatFormatFlags::Style::General:
                    precision = 6;
                    break;
                case FloatFormatFlags::Style::Hex:
                    precision = -1; // 十六进制浮点数使用默认精度（完整精度）
                    break;
                default:
                    precision = -1;
                    break;
            }
        }
        
        // 转换到字符串
        switch (flags.style)
        {
            case FloatFormatFlags::Style::Fixed:
            case FloatFormatFlags::Style::Percent:
                if (precision >= 0)
                {
                    result = std::to_chars(buffer, buffer + buffer_size, value, 
                                         std::chars_format::fixed, precision);
                }
                else
                {
                    result = std::to_chars(buffer, buffer + buffer_size, value, 
                                         std::chars_format::fixed);
                }
                break;
                
            case FloatFormatFlags::Style::Scientific:
                if (precision >= 0)
                {
                    result = std::to_chars(buffer, buffer + buffer_size, value, 
                                         std::chars_format::scientific, precision);
                }
                else
                {
                    result = std::to_chars(buffer, buffer + buffer_size, value, 
                                         std::chars_format::scientific);
                }
                break;
                
            case FloatFormatFlags::Style::General:
                if (precision > 0)
                {
                    result = std::to_chars(buffer, buffer + buffer_size, value, 
                                         std::chars_format::general, precision);
                }
                else if (precision == 0)
                {
                    // 精度0被当作精度1处理
                    result = std::to_chars(buffer, buffer + buffer_size, value, 
                                         std::chars_format::general, 1);
                }
                else
                {
                    result = std::to_chars(buffer, buffer + buffer_size, value, 
                                         std::chars_format::general);
                }
                break;
                
            case FloatFormatFlags::Style::Hex:
                if (precision >= 0)
                {
                    result = std::to_chars(buffer, buffer + buffer_size, value, 
                                         std::chars_format::hex, precision);
                }
                else
                {
                    // 使用默认精度（完整精度）
                    result = std::to_chars(buffer, buffer + buffer_size, value, 
                                         std::chars_format::hex);
                }
                break;
                
            default:
                // 默认格式
                result = std::to_chars(buffer, buffer + buffer_size, value);
                break;
        }
        
        SKR_VERIFY(result.ec == std::errc() && "Failed to convert float to string!");
        length = result.ptr - buffer;
        
        // 后处理：大小写转换
        
        if (flags.uppercase)
        {
            for (size_t i = 0; i < length; ++i)
            {
                if (buffer[i] == 'e') buffer[i] = 'E';
                else if (buffer[i] == 'x') buffer[i] = 'X';
                else if (buffer[i] == 'p') buffer[i] = 'P';
                else if (buffer[i] >= 'a' && buffer[i] <= 'f')
                    buffer[i] = buffer[i] - 'a' + 'A';
            }
        }
        
        // 添加 # 标志的效果（总是显示小数点）
        if (flags.show_point && flags.style != FloatFormatFlags::Style::Hex)
        {
            bool has_point = false;
            bool has_exponent = false;
            size_t exp_pos = 0;
            
            for (size_t i = 0; i < length; ++i)
            {
                if (buffer[i] == '.')
                {
                    has_point = true;
                }
                else if (buffer[i] == 'e' || buffer[i] == 'E')
                {
                    has_exponent = true;
                    exp_pos = i;
                    break;
                }
            }
            
            if (!has_point)
            {
                if (has_exponent)
                {
                    // 在指数前插入小数点
                    std::memmove(buffer + exp_pos + 1, buffer + exp_pos, length - exp_pos);
                    buffer[exp_pos] = '.';
                    length++;
                }
                else if (length < buffer_size - 2)
                {
                    // 对于 g 格式，# 标志应该保留尾随零
                    if (flags.style == FloatFormatFlags::Style::General && flags.precision >= 1)
                    {
                        // g 格式：添加 ".0" 保留一位小数
                        buffer[length++] = '.';
                        buffer[length++] = '0';
                    }
                    else if (length < buffer_size - 1)
                    {
                        // 其他格式：只添加小数点
                        buffer[length++] = '.';
                    }
                }
            }
        }
        
        // 处理符号
        bool has_sign = false;
        size_t sign_length = 0;
        const skr_char8* sign_str = nullptr;
        
        if (buffer[0] == '-')
        {
            has_sign = true;
            sign_str = u8"-";
            sign_length = 1;
            // 移除负号，稍后再添加
            std::memmove(buffer, buffer + 1, length - 1);
            length--;
        }
        else if (flags.show_positive)
        {
            has_sign = true;
            sign_str = u8"+";
            sign_length = 1;
        }
        else if (flags.space_positive)
        {
            has_sign = true;
            sign_str = u8" ";
            sign_length = 1;
        }
        
        // 百分比格式：添加百分号
        if (flags.style == FloatFormatFlags::Style::Percent && length < buffer_size - 1)
        {
            buffer[length++] = '%';
        }
        
        // 计算总长度和填充
        size_t total_length = length + sign_length;
        size_t padding = 0;
        
        if (flags.width > 0 && total_length < static_cast<size_t>(flags.width))
        {
            padding = flags.width - total_length;
        }
        
        // 输出格式化的结果
        if (flags.left_align)
        {
            // 左对齐
            if (has_sign)
                table.append_u8str(table.string_ptr, sign_str, sign_length);
            table.append_cstr(table.string_ptr, buffer, length);
            table.add_chars(table.string_ptr, flags.fill_char, padding);
        }
        else if (flags.center_align)
        {
            // 居中对齐
            size_t left_padding = padding / 2;
            size_t right_padding = padding - left_padding;
            
            table.add_chars(table.string_ptr, flags.fill_char, left_padding);
            if (has_sign)
                table.append_u8str(table.string_ptr, sign_str, sign_length);
            table.append_cstr(table.string_ptr, buffer, length);
            table.add_chars(table.string_ptr, flags.fill_char, right_padding);
        }
        else if (flags.zero_pad && !flags.left_align && !flags.center_align)
        {
            // 零填充（符号在前）
            if (has_sign)
                table.append_u8str(table.string_ptr, sign_str, sign_length);
            table.add_chars(table.string_ptr, u8'0', padding);
            table.append_cstr(table.string_ptr, buffer, length);
        }
        else
        {
            // 右对齐（默认）
            table.add_chars(table.string_ptr, flags.fill_char, padding);
            if (has_sign)
                table.append_u8str(table.string_ptr, sign_str, sign_length);
            table.append_cstr(table.string_ptr, buffer, length);
        }
    }
}

// 使用 StringFunctionTable 的类型擦除版本
void format_float_erased(const StringFunctionTable& table, double value, const skr_char8* spec_data, size_t spec_size)
{
    auto flags = detail::parse_float_spec(spec_data, spec_size);
    detail::format_float_impl(table, value, flags);
}

void format_float_erased(const StringFunctionTable& table, float value, const skr_char8* spec_data, size_t spec_size)
{
    format_float_erased(table, static_cast<double>(value), spec_data, spec_size);
}

} // namespace skr::container