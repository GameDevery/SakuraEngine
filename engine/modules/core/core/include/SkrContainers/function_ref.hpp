#pragma once
#include "SkrContainersDef/function_ref.hpp"

// rttr
#include "SkrRTTR/type_signature.hpp"
namespace skr
{
static constexpr GUID kFunctionRefGenericId = u8"50ac927f-1a5f-497f-aa5d-b74537d93ca6"_guid;
template <typename Func>
struct TypeSignatureTraits<::skr::FunctionRef<Func>>
{
    inline static constexpr bool is_supported = concepts::WithRTTRTraits<Func>;
    inline static constexpr size_t buffer_size = type_signature_size_v<ETypeSignatureSignal::GenericTypeId> + TypeSignatureTraits<Func>::buffer_size;
    inline static uint8_t* write(uint8_t* pos, uint8_t* end)
    {
        pos = TypeSignatureHelper::write_generic_type_id(pos, end, kFunctionRefGenericId, 1);
        return TypeSignatureTraits<Func>::write(pos, end);
    }
};
} // namespace skr