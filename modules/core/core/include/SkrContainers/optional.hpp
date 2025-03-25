#pragma once
#include "SkrContainersDef/optional.hpp"
#include "SkrRTTR/type_signature.hpp"
#include "SkrBase/types/guid.h"

namespace skr
{
static constexpr GUID kOptionalGenericId = u8"bc48634e-85dd-45ce-a7c7-f85d5bab9680"_guid;
template <typename T>
struct TypeSignatureTraits<::skr::Optional<T>> {
    inline static constexpr size_t buffer_size = type_signature_size_v<ETypeSignatureSignal::GenericTypeId> + TypeSignatureTraits<T>::buffer_size;
    inline static uint8_t*         write(uint8_t* pos, uint8_t* end)
    {
        pos = TypeSignatureHelper::write_generic_type_id(pos, end, kOptionalGenericId, 1);
        return TypeSignatureTraits<T>::write(pos, end);
    }
};
} // namespace skr

#include "SkrSerde/bin_serde.hpp"
namespace skr
{
template <typename T>
struct BinSerde<skr::Optional<T>> {
    inline static bool read(SBinaryReader* r, skr::Optional<T>& v)
    {
        // read size
        bool flag;
        if (!bin_read(r, flag))
        {
            SKR_LOG_FATAL(u8"failed to read string buffer size!");
            return false;
        }
        if (flag)
        {
            T temp;
            if (!bin_read<T>(r, temp))
            {
                SKR_LOG_FATAL(u8"failed to read string buffer size!");
                return false;
            }
            v = skr::Optional<T>(std::move(temp));
        }
        else
        {
            v = skr::Optional<T>();
        }
        return true;
    }
    inline static bool write(SBinaryWriter* w, const skr::Optional<T>& v)
    {
        // write flag
        if (!bin_write(w, v.has_value()))
            return false;
        if (v.has_value())
        {
            if (!bin_write<T>(w, v.value()))
                return false;
        }
        return true;
    }
};
} // namespace skr

#include "SkrSerde/json_serde.hpp"
namespace skr
{
template <typename T>
struct JsonSerde<skr::Optional<T>> {
    inline static bool read(skr::archive::JsonReader* r, skr::Optional<T>& v)
    {
        SKR_EXPECTED_CHECK(r->StartObject(), false);

        SKR_EXPECTED_CHECK(r->Key(u8"flag"), false);
        bool flag;
        if (!json_read<bool>(r, flag))
            return false;

        if (flag)
        {
            SKR_EXPECTED_CHECK(r->Key(u8"value"), false);
            T temp;
            if (!json_read<T>(r, temp))
                return false;
            v = skr::Optional<T>(std::move(temp));
        }
        else
        {
            v = skr::Optional<T>();
        }

        SKR_EXPECTED_CHECK(r->EndObject(), false);

        return true;
    }
    inline static bool write(skr::archive::JsonWriter* w, const skr::Optional<T>& v)
    {
        SKR_EXPECTED_CHECK(w->StartObject(), false);

        SKR_EXPECTED_CHECK(w->Key(u8"flag"), false);
        if (!json_write<bool>(w, v.has_value()))
            return false;
        if (v.has_value())
        {
            SKR_EXPECTED_CHECK(w->Key(u8"value"), false);
            if (!json_write<T>(w, v.value()))
                return false;
        }

        SKR_EXPECTED_CHECK(w->EndObject(), false);

        return true;
    }
};
} // namespace skr