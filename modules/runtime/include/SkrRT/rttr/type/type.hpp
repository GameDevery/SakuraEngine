#pragma once
#include "SkrRT/module.configure.h"
#include <cstdint>
#include "SkrBase/config.h"
#include "SkrRT/rttr/guid.hpp"

namespace skr::rttr
{
enum ETypeCategory
{
    SKR_TYPE_CATEGORY_INVALID,

    SKR_TYPE_CATEGORY_PRIMITIVE,
    SKR_TYPE_CATEGORY_ENUM,
    SKR_TYPE_CATEGORY_RECORD,
    SKR_TYPE_CATEGORY_GENERIC,
};

struct RUNTIME_API Type {
    virtual ~Type() = default;

    // getter
    SKR_INLINE ETypeCategory type_category() const { return _type_category; }
    SKR_INLINE GUID          type_id() const { return _type_id; }
    SKR_INLINE size_t        size() const { return _size; }
    SKR_INLINE size_t        alignment() const { return _alignment; }

    // call functions
    virtual bool call_ctor(void* ptr) const                       = 0;
    virtual bool call_dtor(void* ptr) const                       = 0;
    virtual bool call_copy(void* dst, const void* src) const      = 0;
    virtual bool call_move(void* dst, void* src) const            = 0;
    virtual bool call_assign(void* dst, const void* src) const    = 0;
    virtual bool call_move_assign(void* dst, void* src) const     = 0;
    virtual bool call_hash(const void* ptr, size_t& result) const = 0;

    // TODO. convert
    // TODO. serialize

protected:
    // basic data
    ETypeCategory _type_category = ETypeCategory::SKR_TYPE_CATEGORY_INVALID;
    GUID          _type_id       = {};
    size_t        _size          = 0;
    size_t        _alignment     = 0;
};
} // namespace skr::rttr