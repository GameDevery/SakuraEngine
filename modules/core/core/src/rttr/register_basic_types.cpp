#include "SkrCore/exec_static.hpp"
#include "SkrRTTR/export/export_builder.hpp"
#include "SkrRTTR/iobject.hpp"
#include "SkrRTTR/type.hpp"

// primitive type helper
namespace skr
{
template <typename T>
void primitive_type_loader(RTTRType* type)
{
    type->build_primitive([&](RTTRPrimitiveData* data) {
        data->name      = RTTRTraits<T>::get_name();
        data->type_id   = RTTRTraits<T>::get_guid();
        data->size      = sizeof(T);
        data->alignment = alignof(T);
    });
}

static void primitive_type_loader_void(RTTRType* type)
{
    type->build_primitive([&](RTTRPrimitiveData* data) {
        data->name      = RTTRTraits<void>::get_name();
        data->type_id   = RTTRTraits<void>::get_guid();
        data->size      = 0;
        data->alignment = 0;
    });
}
} // namespace skr

SKR_EXEC_STATIC_CTOR
{
    using namespace skr;

    // int types
    register_type_loader(type_id_of<int8_t>(), &primitive_type_loader<int8_t>);
    register_type_loader(type_id_of<int16_t>(), &primitive_type_loader<int16_t>);
    register_type_loader(type_id_of<int32_t>(), &primitive_type_loader<int32_t>);
    register_type_loader(type_id_of<int64_t>(), &primitive_type_loader<int64_t>);
    register_type_loader(type_id_of<uint8_t>(), &primitive_type_loader<uint8_t>);
    register_type_loader(type_id_of<uint16_t>(), &primitive_type_loader<uint16_t>);
    register_type_loader(type_id_of<uint32_t>(), &primitive_type_loader<uint32_t>);
    register_type_loader(type_id_of<uint64_t>(), &primitive_type_loader<uint64_t>);

    // bool & void
    register_type_loader(type_id_of<bool>(), &primitive_type_loader<bool>);
    register_type_loader(type_id_of<void>(), &primitive_type_loader_void);

    // float
    register_type_loader(type_id_of<float>(), &primitive_type_loader<float>);
    register_type_loader(type_id_of<double>(), &primitive_type_loader<double>);
};