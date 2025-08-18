#include <SkrRTTR/type_signature.hpp>
#include <SkrRTTR/generic/generic_base.hpp>
#include <SkrRTTR/type.hpp>
#include <SkrRTTR/type_registry.hpp>

namespace skr
{
// to string helper
inline static const uint8_t* _append_modifier(const uint8_t* pos, const uint8_t* end, String& out, ETypeSignatureSignal signal)
{
    switch (signal)
    {
    case ETypeSignatureSignal::Const: {
        pos = TypeSignatureHelper::read_const(pos, end);
        out = skr::format(u8"const {}", out);
        break;
    }
    case ETypeSignatureSignal::Pointer: {
        pos = TypeSignatureHelper::read_pointer(pos, end);
        out = skr::format(u8"* {}", out);
        break;
    }
    case ETypeSignatureSignal::Ref: {
        pos = TypeSignatureHelper::read_ref(pos, end);
        out = skr::format(u8"& {}", out);
        break;
    }
    case ETypeSignatureSignal::RValueRef: {
        pos = TypeSignatureHelper::read_rvalue_ref(pos, end);
        out = skr::format(u8"&& {}", out);
        break;
    }
    case ETypeSignatureSignal::ArrayDim: {
        uint32_t dim;
        pos = TypeSignatureHelper::read_array_dim(pos, end, dim);
        out = skr::format(u8"[{}] {}", dim, out);
        break;
    }
    default:
        SKR_UNREACHABLE_CODE()
    }
    return pos;
}
inline static const uint8_t* _append_data(const uint8_t* pos, const uint8_t* end, String& out, ETypeSignatureSignal signal)
{
    switch (signal)
    {
    case ETypeSignatureSignal::Bool: {
        bool value;
        pos = TypeSignatureHelper::read_bool(pos, end, value);
        out = skr::format(u8"{}{}", out, (value ? "true" : "false"));
        break;
    }
    case ETypeSignatureSignal::UInt8: {
        uint8_t value;
        pos = TypeSignatureHelper::read_uint8(pos, end, value);
        out = skr::format(u8"{}{}", out, value);
        break;
    }
    case ETypeSignatureSignal::UInt16: {
        uint16_t value;
        pos = TypeSignatureHelper::read_uint16(pos, end, value);
        out = skr::format(u8"{}{}", out, value);
        break;
    }
    case ETypeSignatureSignal::UInt32: {
        uint32_t value;
        pos = TypeSignatureHelper::read_uint32(pos, end, value);
        out = skr::format(u8"{}{}", out, value);
        break;
    }
    case ETypeSignatureSignal::UInt64: {
        uint64_t value;
        pos = TypeSignatureHelper::read_uint64(pos, end, value);
        out = skr::format(u8"{}{}", out, value);
        break;
    }
    case ETypeSignatureSignal::Int8: {
        int8_t value;
        pos = TypeSignatureHelper::read_int8(pos, end, value);
        out = skr::format(u8"{}{}", out, value);
        break;
    }
    case ETypeSignatureSignal::Int16: {
        int16_t value;
        pos = TypeSignatureHelper::read_int16(pos, end, value);
        out = skr::format(u8"{}{}", out, value);
        break;
    }
    case ETypeSignatureSignal::Int32: {
        int32_t value;
        pos = TypeSignatureHelper::read_int32(pos, end, value);
        out = skr::format(u8"{}{}", out, value);
        break;
    }
    case ETypeSignatureSignal::Int64: {
        int64_t value;
        pos = TypeSignatureHelper::read_int64(pos, end, value);
        out = skr::format(u8"{}{}", out, value);
        break;
    }
    case ETypeSignatureSignal::Float: {
        float value;
        pos = TypeSignatureHelper::read_float(pos, end, value);
        out = skr::format(u8"{}{}", out, value);
        break;
    }
    case ETypeSignatureSignal::Double: {
        double value;
        pos = TypeSignatureHelper::read_double(pos, end, value);
        out = skr::format(u8"{}{}", out, value);
        break;
    }
    default:
        SKR_UNREACHABLE_CODE()
    }
    return pos;
}

String TypeSignatureHelper::signal_to_string(const uint8_t* pos, const uint8_t* end)
{
    auto signature_type = validate_complete_signature(pos, end);
    SKR_ASSERT(signature_type != ETypeSignatureSignal::None && "Invalid type signature");

    // append modifier
    String modifier;
    while (true)
    {
        auto signal = peek_signal(pos, end);
        if (is_modifier(signal))
        {
            pos = _append_modifier(pos, end, modifier, signal);
        }
        else
        {
            break;
        }
    }
    if (!modifier.is_empty())
    {
        modifier.trim_end();
        modifier.append_at(0, u8" ");
    }

    // append type data
    switch (signature_type)
    {
    case ETypeSignatureSignal::TypeId: {
        // get type name
        GUID type_id;
        pos       = read_type_id(pos, end, type_id);
        auto type = get_type_from_guid(type_id);
        if (type)
        {
            return skr::format(u8"{}{}", type->full_name(), modifier);
        }
        else
        {
            return skr::format(u8"${}${}", type_id, modifier);
        }
    }
    case ETypeSignatureSignal::GenericTypeId: {
        GUID     generic_id;
        uint32_t data_count;
        pos = read_generic_type_id(pos, end, generic_id, data_count);

        // get generic type name
        auto generic_name = get_generic_name(generic_id);
        if (generic_name.is_empty())
        {
            generic_name = skr::format(u8"${}$", generic_id);
        }

        // build generic data list
        String generic_data_list;
        auto   next_pos = pos;
        for (uint32_t i = 0; i < data_count; ++i)
        {
            pos      = next_pos;
            next_pos = jump_next_type_or_data(pos, end);

            // append separator
            if (i != 0)
            {
                generic_data_list.append(u8",");
            }

            // append data
            auto data_signal = peek_signal(pos, end);
            if (is_data(data_signal))
            {
                _append_data(pos, next_pos, generic_data_list, data_signal);
            }
            else
            {
                generic_data_list.append(signal_to_string(pos, next_pos));
            }
        }
        return skr::format(u8"{}<{}>{}", generic_name, generic_data_list, modifier);
    }
    case ETypeSignatureSignal::FunctionSignature: {
        uint32_t param_count;
        pos = read_function_signature(pos, end, param_count);

        // build return
        String return_name;
        auto   next_pos = jump_next_type_or_data(pos, end);
        return_name     = signal_to_string(pos, next_pos);

        // append parameters
        String param_list;
        for (uint32_t i = 0; i < param_count; ++i)
        {
            pos      = next_pos;
            next_pos = jump_next_type_or_data(pos, end);

            // append separator
            if (i != 0)
            {
                param_list.append(u8",");
            }

            // append data
            param_list.append(signal_to_string(pos, next_pos));
        }

        if (modifier.is_empty())
        {
            return skr::format(u8"{}({})", return_name, param_list);
        }
        else
        {
            return skr::format(u8"{}({})({})", return_name, modifier, param_list);
        }
    }
    default:
        SKR_UNREACHABLE_CODE()
        return {};
    }
}
} // namespace skr