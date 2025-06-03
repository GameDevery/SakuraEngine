#pragma once
#include <SkrRTTR/type.hpp>
#include <SkrCore/log.hpp>
#include <SkrContainers/optional.hpp>
#include <SkrContainers/vector.hpp>
#include <SkrCore/memory/rc.hpp>

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
    InvalidType,
    NoFunctional
};
struct IGenericBase {
    SKR_RC_INTEFACE()
    virtual ~IGenericBase() = default;

    // get type info
    virtual EGenericKind kind() const = 0;
    virtual GUID         guid() const = 0;

    // get utils
    virtual MemoryTraitsData memory_traits_data() const = 0;

    // operations, used for generic container algorithms
    virtual Expected<EGenericError>         copy(void* dst, const void* src) const        = 0;
    virtual Expected<EGenericError>         move(void* dst, void* src) const              = 0;
    virtual Expected<EGenericError, size_t> hash(const void* src)                         = 0;
    virtual Expected<EGenericError, bool>   equal(const void* lhs, const void* rhs) const = 0;
};

// TODO. Generic Registry
//   register generic handlers
//   make generic from type signature
} // namespace skr