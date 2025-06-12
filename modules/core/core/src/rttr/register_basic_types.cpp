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
        RTTRPrimitiveBuilder<T> builder(data);
        builder.basic_info();
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

template <typename T>
void minimal_type_loader_record(RTTRType* type)
{
    type->build_record([](RTTRRecordData* data) {
        RTTRRecordBuilder<T> builder(data);
        builder.basic_info();
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

#define REG_MINIMAL_TYPE_LOADER(__TYPE) register_type_loader(type_id_of<__TYPE>(), &minimal_type_loader_record<__TYPE>)

    // guid & md5
    REG_MINIMAL_TYPE_LOADER(::skr::GUID);
    REG_MINIMAL_TYPE_LOADER(::skr::MD5);

    // string types
    REG_MINIMAL_TYPE_LOADER(::skr::String);
    REG_MINIMAL_TYPE_LOADER(::skr::StringView);

    // math misc types
    REG_MINIMAL_TYPE_LOADER(::skr::RotatorF);
    REG_MINIMAL_TYPE_LOADER(::skr::RotatorD);
    REG_MINIMAL_TYPE_LOADER(::skr::QuatF);
    REG_MINIMAL_TYPE_LOADER(::skr::QuatD);
    REG_MINIMAL_TYPE_LOADER(::skr::TransformF);
    REG_MINIMAL_TYPE_LOADER(::skr::TransformD);

    // math vector types
    REG_MINIMAL_TYPE_LOADER(::skr::float2);
    REG_MINIMAL_TYPE_LOADER(::skr::float3);
    REG_MINIMAL_TYPE_LOADER(::skr::float4);
    REG_MINIMAL_TYPE_LOADER(::skr::double2);
    REG_MINIMAL_TYPE_LOADER(::skr::double3);
    REG_MINIMAL_TYPE_LOADER(::skr::double4);
    REG_MINIMAL_TYPE_LOADER(::skr::int2);
    REG_MINIMAL_TYPE_LOADER(::skr::int3);
    REG_MINIMAL_TYPE_LOADER(::skr::int4);
    REG_MINIMAL_TYPE_LOADER(::skr::uint2);
    REG_MINIMAL_TYPE_LOADER(::skr::uint3);
    REG_MINIMAL_TYPE_LOADER(::skr::uint4);
    REG_MINIMAL_TYPE_LOADER(::skr::long2);
    REG_MINIMAL_TYPE_LOADER(::skr::long3);
    REG_MINIMAL_TYPE_LOADER(::skr::long4);
    REG_MINIMAL_TYPE_LOADER(::skr::ulong2);
    REG_MINIMAL_TYPE_LOADER(::skr::ulong3);
    REG_MINIMAL_TYPE_LOADER(::skr::ulong4);
    REG_MINIMAL_TYPE_LOADER(::skr::bool2);
    REG_MINIMAL_TYPE_LOADER(::skr::bool3);
    REG_MINIMAL_TYPE_LOADER(::skr::bool4);

    // matrix types
    REG_MINIMAL_TYPE_LOADER(::skr::float3x3);
    REG_MINIMAL_TYPE_LOADER(::skr::float4x4);
    REG_MINIMAL_TYPE_LOADER(::skr::double3x3);
    REG_MINIMAL_TYPE_LOADER(::skr::double4x4);

#undef REG_MINIMAL_TYPE_LOADER
};