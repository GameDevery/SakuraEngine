#pragma once
#include <SkrRTTR/type.hpp>
#include <SkrCore/log.hpp>
#include <SkrContainers/optional.hpp>
#include <SkrContainers/vector.hpp>
#include <SkrCore/memory/rc.hpp>
#include <SkrContainers/not_null.hpp>

namespace skr
{
// generic interface
enum class EGenericKind
{
    Generic,
    Type,
};
enum class EGenericError
{
    Unknown,
    InvalidType,   // generic type is invalid
    NoSuchFeature, // no such feature
    ShouldNotCall, // should not call this function
};
struct SKR_CORE_API IGenericBase {
    SKR_RC_INTEFACE()
    virtual ~IGenericBase();

    // get type info
    virtual EGenericKind kind() const = 0;
    virtual GUID         id() const   = 0; // type or generic id, depends on kind

    // get utils
    virtual MemoryTraitsData memory_traits_data() const = 0;

    // basic info
    virtual uint64_t size() const      = 0;
    virtual uint64_t alignment() const = 0;

    // operations, used for generic container algorithms
    virtual Expected<EGenericError>         default_ctor(void* dst) const                 = 0;
    virtual Expected<EGenericError>         dtor(void* dst) const                         = 0;
    virtual Expected<EGenericError>         copy(void* dst, const void* src) const        = 0;
    virtual Expected<EGenericError>         move(void* dst, void* src) const              = 0;
    virtual Expected<EGenericError>         assign(void* dst, const void* src) const      = 0;
    virtual Expected<EGenericError>         move_assign(void* dst, void* src) const       = 0;
    virtual Expected<EGenericError, bool>   equal(const void* lhs, const void* rhs) const = 0;
    virtual Expected<EGenericError, size_t> hash(const void* src) const                   = 0;
};

struct SKR_CORE_API GenericMemoryOps {
    static Expected<EGenericError> construct(
        IGenericBase*    generic,
        MemoryTraitsData traits,
        void*            dst,
        uint64_t         count = 1
    );
    static Expected<EGenericError> destruct(
        IGenericBase*    generic,
        MemoryTraitsData traits,
        void*            dst,
        uint64_t         count = 1
    );
    static Expected<EGenericError> copy(
        IGenericBase*    generic,
        MemoryTraitsData traits,
        void*            dst,
        const void*      src,
        uint64_t         count = 1
    );
    static Expected<EGenericError> move(
        IGenericBase*    generic,
        MemoryTraitsData traits,
        void*            dst,
        void*            src,
        uint64_t         count = 1
    );
    static Expected<EGenericError> assign(
        IGenericBase*    generic,
        MemoryTraitsData traits,
        void*            dst,
        const void*      src,
        uint64_t         count = 1
    );
    static Expected<EGenericError> move_assign(
        IGenericBase*    generic,
        MemoryTraitsData traits,
        void*            dst,
        void*            src,
        uint64_t         count = 1
    );
    static Expected<EGenericError, bool> equal(
        IGenericBase*    generic,
        MemoryTraitsData traits,
        const void*      lhs,
        const void*      rhs,
        uint64_t         count = 1
    );
};

// TODO. 支持一重指针/引用等
enum class EGenericTypeFlag : uint8_t
{
    None      = 0,
    Const     = 1 << 0, // const pointer
    Pointer   = 1 << 1, // pointer type
    Ref       = 1 << 2, // reference type
    RValueRef = 1 << 3, // rvalue reference type
};
// TODO. 缓存函数，减少查找开销
struct SKR_CORE_API GenericType final : IGenericBase {
    SKR_RC_IMPL(override final)

    // ctor & dtor
    GenericType(not_null<RTTRType*> type, EGenericTypeFlag flag = EGenericTypeFlag::None);

    // getter
    EGenericTypeFlag flag() const;
    bool             is_const() const;
    bool             is_pointer() const;
    bool             is_ref() const;
    bool             is_rvalue_ref() const;
    bool             is_decayed_pointer() const;

    //===> IGenericBase API
    // get type info
    EGenericKind kind() const override final;
    GUID         id() const override final;

    // get utils
    MemoryTraitsData memory_traits_data() const override final;

    // basic info
    uint64_t size() const override final;
    uint64_t alignment() const override final;

    // operations, used for generic container algorithms
    Expected<EGenericError>         default_ctor(void* dst) const override final;
    Expected<EGenericError>         dtor(void* dst) const override final;
    Expected<EGenericError>         copy(void* dst, const void* src) const override final;
    Expected<EGenericError>         move(void* dst, void* src) const override final;
    Expected<EGenericError>         assign(void* dst, const void* src) const override final;
    Expected<EGenericError>         move_assign(void* dst, void* src) const override final;
    Expected<EGenericError, bool>   equal(const void* lhs, const void* rhs) const override final;
    Expected<EGenericError, size_t> hash(const void* src) const override final;
    //===> IGenericBase API
private:
    RTTRType*        _type = nullptr;
    EGenericTypeFlag _flag = EGenericTypeFlag::None;
};

// TODO. Generic Registry
//   register generic handlers
//   make generic from type signature
} // namespace skr