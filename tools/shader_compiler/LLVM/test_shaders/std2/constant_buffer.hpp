#pragma once
#include "./../std/std.hpp"

namespace skr::shader {

namespace concepts
{
    template<typename T>
    concept struct_type = __is_class(T) && !__is_union(T) && !__is_enum(T) && !is_vec_or_matrix_v<T> && !is_arithmetic_v<T>;
}

template <concepts::struct_type Type>
struct [[builtin("constant_buffer")]] ConstantBuffer : public Type
{
    
};

} // namespace skr::shader