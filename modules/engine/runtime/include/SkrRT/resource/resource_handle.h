#pragma once
#include "SkrBase/config/key_words.h"
#include "SkrRTTR/rttr_traits.hpp"

enum ESkrLoadingStatus : uint32_t;
struct SResourceRecord;
enum ESkrRequesterType
{
    SKR_REQUESTER_ENTITY = 0,
    SKR_REQUESTER_DEPENDENCY = 1,
    SKR_REQUESTER_SYSTEM = 2,
    SKR_REQUESTER_SCRIPT = 3,
    SKR_REQUESTER_UNKNOWN = 4
};
struct lua_State;
typedef struct SResourceHandle
{
    union
    {
        skr_guid_t guid;
        struct
        {
            uint32_t padding;     // zero, flag for resolved or not
            uint32_t requesterId; // requester id
            // since resource record is allocated with alignment 8, the lower 3 bits should always be zero
            // so we put requester type into it
            uint64_t pointer; // resource record ptr & requester type
        };
    };
#if defined(__cplusplus)
    SKR_RUNTIME_API SResourceHandle();
    SKR_RUNTIME_API ~SResourceHandle();
    SKR_RUNTIME_API SResourceHandle(const skr_guid_t& other);
    SKR_RUNTIME_API SResourceHandle(const SResourceHandle& other);
    SKR_RUNTIME_API SResourceHandle(const SResourceHandle& other, uint64_t requester, ESkrRequesterType requesterType);
    SKR_RUNTIME_API SResourceHandle(SResourceHandle&& other);
    SKR_RUNTIME_API SResourceHandle& operator=(const SResourceHandle& other);
    SKR_RUNTIME_API SResourceHandle& operator=(const skr_guid_t& other);
    SKR_RUNTIME_API SResourceHandle& operator=(SResourceHandle&& other);
    SKR_RUNTIME_API void set_ptr(void* ptr);
    SKR_RUNTIME_API void set_guid(const skr_guid_t& guid);
    SKR_RUNTIME_API bool is_resolved() const;
    SKR_RUNTIME_API void* get_resolved(bool requireInstalled = true) const;
    SKR_RUNTIME_API skr_guid_t get_serialized() const;
    SKR_RUNTIME_API void resolve(bool requireInstalled, uint64_t requester, ESkrRequesterType requesterType);
    void resolve(bool requireInstalled, struct sugoi_storage_t* requester)
    {
        resolve(requireInstalled, (uint64_t)requester, SKR_REQUESTER_ENTITY);
    }
    SKR_RUNTIME_API void unload();
    SKR_RUNTIME_API skr_guid_t get_guid() const;
    SKR_RUNTIME_API skr_guid_t get_type() const;
    SKR_RUNTIME_API void* get_ptr() const;
    SKR_RUNTIME_API bool is_null() const;
    SKR_RUNTIME_API void reset();
    SResourceHandle clone(uint64_t requester, ESkrRequesterType requesterType)
    {
        return { *this, requester, requesterType };
    }
    SResourceHandle clone(struct sugoi_storage_t* requester)
    {
        return { *this, (uint64_t)requester, SKR_REQUESTER_ENTITY };
    }
    SKR_RUNTIME_API uint32_t get_requester_id() const;
    SKR_RUNTIME_API ESkrRequesterType get_requester_type() const;
    // if resolve is false, then unresolve handle will always return SKR_LOADING_STATUS_UNLOADED
    SKR_RUNTIME_API ESkrLoadingStatus get_status(bool resolve = false) const;
    SKR_RUNTIME_API SResourceRecord* get_record() const;
    SKR_RUNTIME_API void set_record(SResourceRecord* record);
    SKR_RUNTIME_API void set_resolved(SResourceRecord* record, uint32_t requesterId, ESkrRequesterType requesterType);
#endif
} SResourceHandle;

SKR_EXTERN_C SKR_RUNTIME_API int skr_is_resource_resolved(SResourceHandle* handle);
SKR_EXTERN_C SKR_RUNTIME_API void skr_get_resource_guid(SResourceHandle* handle, skr_guid_t* guid);
SKR_EXTERN_C SKR_RUNTIME_API void skr_get_resource(SResourceHandle* handle, void** guid);

#if defined(__cplusplus)
#include "SkrSerde/bin_serde.hpp"
#include "SkrSerde/json_serde.hpp"

#define SKR_RESOURCE_HANDLE(type) skr::resource::AsyncResource<type>
#define SKR_RESOURCE_FIELD(type, name) skr::resource::AsyncResource<type> name

SKR_RTTR_TYPE(SResourceHandle, "A9E0CE3D-5E9B-45F1-AC28-B882885C63AB");
namespace skr::resource
{
using ResourceHandle = SResourceHandle;
template <class T>
struct AsyncResource : ResourceHandle
{
    using SResourceHandle::SResourceHandle;

    // TODO: T* resolve
    T* get_resolved(bool requireInstalled = true) const
    {
        return (T*)ResourceHandle::get_resolved(requireInstalled);
    }
    T* get_ptr() const
    {
        return (T*)ResourceHandle::get_ptr();
    }
    AsyncResource clone(uint64_t requester, ESkrRequesterType requesterType)
    {
        return { *this, requester, requesterType };
    }
    AsyncResource clone(struct sugoi_storage_t* requester)
    {
        return { *this, (uint64_t)requester, SKR_REQUESTER_ENTITY };
    }
};
} // namespace skr::resource

// bin serde
namespace skr
{
template <>
struct BinSerde<skr::resource::ResourceHandle>
{
    inline static bool read(SBinaryReader* r, skr::resource::ResourceHandle& v)
    {
        skr_guid_t guid;
        if (!bin_read(r, guid))
        {
            SKR_LOG_FATAL(u8"failed to read resource handle guid! ret code: %d", -1);
            return false;
        }
        v.set_guid(guid);
        return true;
    }
    inline static bool write(SBinaryWriter* w, const skr::resource::ResourceHandle& v)
    {
        return bin_write(w, v.get_serialized());
    }
};

template <class T>
struct BinSerde<skr::resource::AsyncResource<T>>
{
    inline static bool read(SBinaryReader* archive, skr::resource::AsyncResource<T>& handle)
    {
        skr_guid_t guid;
        if (!bin_read(archive, (guid))) return false;
        handle.set_guid(guid);
        return true;
    }
    inline static bool write(SBinaryWriter* binary, const skr::resource::AsyncResource<T>& handle)
    {
        const auto& hdl = static_cast<const skr::resource::ResourceHandle&>(handle);
        return bin_write(binary, hdl);
    }
};

template <>
struct JsonSerde<skr::resource::ResourceHandle>
{
    inline static bool read(skr::archive::JsonReader* r, skr::resource::ResourceHandle& v)
    {
        SkrZoneScopedN("JsonSerde<skr::resource::ResourceHandle>::read");
        skr::String view;
        SKR_EXPECTED_CHECK(r->String(view), false);
        {
            skr_guid_t guid;
            if (!skr::guid_from_sv(view.c_str(), guid))
                return false;
            v.set_guid(guid);
        }
        return true;
    }
    inline static bool write(skr::archive::JsonWriter* w, const skr::resource::ResourceHandle& v)
    {
        return json_write<skr_guid_t>(w, v.get_serialized());
    }
};
template <class T>
struct JsonSerde<skr::resource::AsyncResource<T>>
{
    inline static bool read(skr::archive::JsonReader* r, skr::resource::AsyncResource<T>& v)
    {
        return json_read<skr::resource::ResourceHandle>(r, (skr::resource::ResourceHandle&)v);
    }
    inline static bool write(skr::archive::JsonWriter* w, const skr::resource::AsyncResource<T>& v)
    {
        return json_write<skr::resource::ResourceHandle>(w, (const skr::resource::ResourceHandle&)v);
    }
};
} // namespace skr
#else
    #define SKR_RESOURCE_HANDLE(type) SResourceHandle
    #define SKR_RESOURCE_FIELD(type, name) SResourceHandle name
#endif