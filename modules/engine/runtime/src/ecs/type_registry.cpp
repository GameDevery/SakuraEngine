#include "SkrBase/misc/make_zeroed.hpp"
#include "SkrRT/ecs/sugoi.h"
#include "SkrRT/ecs/type_index.hpp"

#include "./pool.hpp"
#include "./impl/type_registry.hpp"
#include <string.h>

#if SKR_PLAT_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
#endif

#if __SSE2__
    #include <emmintrin.h>
    #define SUGOI_MASK_ALIGN alignof(__m128i)
#else
    #define SUGOI_MASK_ALIGN alignof(uint32_t)
#endif

namespace sugoi
{
TypeRegistry::Impl::Impl(pool_t& pool)
    : nameArena(pool)
{
    using namespace skr::literals;
    {
        SKR_ASSERT(descriptions.size() == kDisableComponent.index());
        auto desc = make_zeroed<type_description_t>();
        desc.guid = u8"{B68B1CAB-98FF-4298-A22E-68B404034B1B}"_guid;
        desc.name = u8"disable";
        desc.size = 0;
        desc.elementSize = 0;
        desc.alignment = 0;
        desc.entityFieldsCount = 0;
        desc.flags = 0;
        descriptions.add(desc);
        guid2type.emplace(desc.guid, kDisableComponent);
        name2type.emplace(desc.name, kDisableComponent);
    }
    {
        SKR_ASSERT(descriptions.size() == kDeadComponent.index());
        auto desc = make_zeroed<type_description_t>();
        desc.guid = u8"{C0471B12-5462-48BB-B8C4-9983036ECC6C}"_guid;
        desc.name = u8"dead";
        desc.size = 0;
        desc.elementSize = 0;
        desc.alignment = 0;
        desc.entityFieldsCount = 0;
        desc.flags = 0;
        descriptions.add(desc);
        guid2type.emplace(desc.guid, kDeadComponent);
        name2type.emplace(desc.name, kDeadComponent);
    }
    {
        SKR_ASSERT(descriptions.size() == kLinkComponent.index());
        auto desc = make_zeroed<type_description_t>();
        desc.guid = u8"{54BD68D5-FD66-4DBE-85CF-70F535C27389}"_guid;
        desc.name = u8"sugoi::link_comp_t";
        desc.size = sizeof(sugoi_entity_t) * kLinkComponentSize;
        desc.elementSize = sizeof(sugoi_entity_t);
        desc.alignment = alignof(sugoi_entity_t);
        desc.entityFieldsCount = 1;
        entityFields.add(0);
        desc.entityFields = 0;
        desc.flags = 0;
        descriptions.add(desc);
        guid2type.emplace(desc.guid, kLinkComponent);
        name2type.emplace(desc.name, kLinkComponent);
    }
    {
        SKR_ASSERT(descriptions.size() == kMaskComponent.index());
        auto desc = make_zeroed<type_description_t>();
        desc.guid = u8"{B68B1CAB-98FF-4298-A22E-68B404034B1B}"_guid;
        desc.name = u8"sugoi::mask_comp_t";
        desc.size = sizeof(sugoi_mask_comp_t);
        desc.elementSize = 0;
        desc.alignment = SUGOI_MASK_ALIGN;
        desc.entityFieldsCount = 0;
        desc.flags = 0;
        descriptions.add(desc);
        guid2type.emplace(desc.guid, kMaskComponent);
        name2type.emplace(desc.name, kMaskComponent);
    }
    {
        SKR_ASSERT(descriptions.size() == kGuidComponent.index());
        auto desc = make_zeroed<type_description_t>();
        desc.guid = u8"{565FBE87-6309-4DF7-9B3F-C61B67B38BB3}"_guid;
        desc.name = u8"sugoi::guid_comp_t";
        desc.size = sizeof(sugoi_guid_t);
        desc.elementSize = 0;
        desc.alignment = alignof(sugoi_guid_t);
        desc.entityFieldsCount = 0;
        desc.flags = 0;
        descriptions.add(desc);
        guid2type.emplace(desc.guid, kGuidComponent);
        name2type.emplace(desc.name, kGuidComponent);
    }
    {
        SKR_ASSERT(descriptions.size() == kDirtyComponent.index());
        auto desc = make_zeroed<type_description_t>();
        desc.guid = u8"{A55D73D3-D41C-4683-89E1-8B211C115303}"_guid;
        desc.name = u8"sugoi::dirty_comp_t";
        desc.size = sizeof(sugoi_dirty_comp_t);
        desc.elementSize = 0;
        desc.alignment = SUGOI_MASK_ALIGN;
        desc.entityFieldsCount = 0;
        desc.flags = 0;
        descriptions.add(desc);
        guid2type.emplace(desc.guid, kDirtyComponent);
        name2type.emplace(desc.name, kDirtyComponent);
    }
}

type_index_t TypeRegistry::Impl::register_type(const type_description_t& inDesc)
{
    type_description_t desc = inDesc;
    if (!desc.name)
    {
        return kInvalidTypeIndex;
    }
    else
    {
        if (name2type.count(desc.name))
            return kInvalidTypeIndex;
        auto len = strlen((const char*)desc.name);
        auto name = (char8_t*)nameArena.allocate(len + 1, 1);
        memcpy(name, desc.name, len + 1);
        desc.name = name;
    }
    if (desc.entityFields != 0)
    {
        intptr_t* efs = (intptr_t*)desc.entityFields;
        desc.entityFields = entityFields.size();
        for (uint32_t i = 0; i < desc.entityFieldsCount; ++i)
            entityFields.add(efs[i]);
    }
    bool tag = false;
    bool pin = false;
    bool buffer = false;
    bool chunk = false;
    if (desc.size == 0)
        tag = true;
    if (desc.elementSize != 0)
        buffer = true;
    pin = (desc.flags & SUGOI_TYPE_FLAG_PIN) != 0;
    chunk = (desc.flags & SUGOI_TYPE_FLAG_CHUNK) != 0;
    SKR_ASSERT(!(chunk && pin));
    SKR_ASSERT(!(chunk && tag));
    type_index_t index{ (TIndex)descriptions.size(), pin, buffer, tag, chunk };
    
    auto i = guid2type.find(inDesc.guid);
    if (i != guid2type.end())
    {
        auto& oldDesc = descriptions[i->second];
        bool capable = true;
        {
            if (oldDesc.size != desc.size)
                capable = false;
            if (oldDesc.alignment != desc.alignment)
                capable = false;
            if (oldDesc.elementSize != desc.elementSize)
                capable = false;
            if (oldDesc.entityFieldsCount != desc.entityFieldsCount)
                capable = false;
            if (oldDesc.flags != desc.flags)
                capable = false;
            if (oldDesc.entityFieldsCount != desc.entityFieldsCount)
                capable = false;
            if (oldDesc.entityFields != desc.entityFields)
                capable = false;
        }
        SKR_ASSERT(capable);
        name2type.emplace(desc.name, i->second);
        //old callback pointers maybe invalid after a reload of the dll
        oldDesc = desc;
        return i->second;
    }
    descriptions.add(desc);
    guid2type.emplace(desc.guid, index);
    name2type.emplace(desc.name, index);
    return index;
}

type_index_t TypeRegistry::Impl::get_type(const guid_t& guid)
{
    auto i = guid2type.find(guid);
    if (i != guid2type.end())
        return i->second;
    return kInvalidTypeIndex;
}

type_index_t TypeRegistry::Impl::get_type(skr::StringView name)
{
    auto i = name2type.find(name);
    if (i != name2type.end())
        return i->second;
    return kInvalidTypeIndex;
}

const sugoi_type_description_t* TypeRegistry::Impl::get_type_desc(sugoi_type_index_t idx)
{
    return &descriptions[sugoi::type_index_t(idx).index()];
}

void TypeRegistry::Impl::foreach_types(sugoi_type_callback_t callback, void* u)
{
    for(auto& pair : name2type)
        callback(u, pair.second);
}

intptr_t TypeRegistry::Impl::map_entity_field(intptr_t p)
{
    return entityFields[p];
}

guid_t TypeRegistry::Impl::make_guid()
{
    guid_t guid;
    skr_make_guid(&guid);
    return guid;
}

TypeRegistry::TypeRegistry(Impl& impl)
    : impl(impl)
{
}


type_index_t TypeRegistry::register_type(const sugoi_type_description_t& desc)
{
    return impl.register_type(desc);
}

type_index_t TypeRegistry::get_type(const guid_t& guid)
{
    return impl.get_type(guid);
}

type_index_t TypeRegistry::get_type(skr::StringView name)
{
    return impl.get_type(name);
}

const sugoi_type_description_t* TypeRegistry::get_type_desc(sugoi_type_index_t idx)
{
    return impl.get_type_desc(idx);
}

void TypeRegistry::foreach_types(sugoi_type_callback_t callback, void* u)
{
    impl.foreach_types(callback, u);
}

intptr_t TypeRegistry::map_entity_field(intptr_t p)
{
    return impl.map_entity_field(p);
}

guid_t TypeRegistry::make_guid()
{
    return impl.make_guid();
}

} // namespace sugoi

extern "C" {

void sugoi_make_guid(skr_guid_t* guid)
{
    *guid = sugoi::TypeRegistry::get().make_guid();
}

uint16_t sugoiT_get_buffer_size(uint16_t elementSize, uint16_t elementAlign, uint16_t count)
{
    auto start = elementAlign * ((sizeof(sugoi_array_comp_t) + elementAlign - 1) / elementAlign);
    return start + elementSize * count;
}

sugoi_type_index_t sugoiT_register_type(sugoi_type_description_t* description)
{
    return sugoi::TypeRegistry::get().register_type(*description);
}

sugoi_type_index_t sugoiT_get_type(const sugoi_guid_t* guid)
{
    return sugoi::TypeRegistry::get().get_type(*guid);
}

sugoi_type_index_t sugoiT_get_type_by_name(const char8_t* name)
{
    return sugoi::TypeRegistry::get().get_type(name);
}

const sugoi_type_description_t* sugoiT_get_desc(sugoi_type_index_t idx)
{
    return sugoi::TypeRegistry::get().get_type_desc(idx);
}

void sugoiT_get_types(sugoi_type_callback_t callback, void* u)
{
    sugoi::TypeRegistry::get().foreach_types(callback, u);
}
}