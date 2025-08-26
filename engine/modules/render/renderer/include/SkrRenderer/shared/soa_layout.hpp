#pragma once
#ifdef __CPPSL__
    #include "detail/type_usings.hxx" // IWYU pragma: export
#else
    #include "detail/type_usings.hpp" // IWYU pragma: export
#endif

namespace skr
{

using uint32_t = skr::data_layout::uint32_t;
using AddressType = skr::data_layout::AddressType;

template <AddressType v, AddressType a>
constexpr AddressType align_up = (v + a - 1) & ~(a - 1);

template <typename... Cs>
struct ComponentBundle
{
public:
    static constexpr size_t component_count = sizeof...(Cs);
    static constexpr AddressType size() {
        return size_helper<Cs...>();
    }

private:
    template <typename F, typename... Rs>
    static constexpr AddressType size_helper() {
        constexpr auto aligned = align_up<0, alignof(F)>;
        if constexpr (sizeof...(Rs) == 0) {
            return aligned + sizeof(F);
        } else {
            return size_helper_impl<aligned + sizeof(F), Rs...>();
        }
    }
    template <AddressType Off, typename F, typename... Rs>
    static constexpr AddressType size_helper_impl() {
        constexpr auto aligned = align_up<Off, alignof(F)>;
        if constexpr (sizeof...(Rs) == 0) {
            return aligned + sizeof(F);
        } else {
            return size_helper_impl<aligned + sizeof(F), Rs...>();
        }
    }

public:
    static constexpr AddressType alignment() {
        AddressType max_align = 1;
        ((max_align = (alignof(Cs) > max_align) ? alignof(Cs) : max_align), ...);
        return max_align;
    }
    template <typename T>
    static constexpr bool contains() { return (std::is_same_v<T, Cs> || ...); }
    template <typename T>
    static constexpr AddressType offset_in_bundle() { return offset_helper<T, 0, Cs...>(); }
    
private:
    template <typename T, AddressType Off, typename F, typename... Rs>
    static constexpr AddressType offset_helper()
    {
        constexpr auto aligned = align_up<Off, alignof(F)>;
        if constexpr (std::is_same_v<T, F>) return aligned;
        else if constexpr (sizeof...(Rs) > 0) return offset_helper<T, aligned + sizeof(F), Rs...>();
        else return AddressType(-1);
    }
};

// 类型特征
template <typename T> struct is_bundle : std::false_type {};
template <typename... Ts> struct is_bundle<ComponentBundle<Ts...>> : std::true_type {};
template <typename T> inline constexpr bool is_bundle_v = is_bundle<T>::value;

// 分页布局：支持 SOA 和 AOSOA
template <uint32_t PageSize = 16384, typename... Es>
struct PagedLayout
{
public:
    template <typename... T> struct TypePack {};
    using Elements = TypePack<Es...>;
    static constexpr uint32_t page_size = PageSize;
    static_assert(PageSize != ~0, "bad page size!");
    
    template <typename E>
    static constexpr AddressType elem_size() {
        if constexpr (is_bundle_v<E>) {
            return E::size();
        } else {
            return sizeof(E);
        }
    }
    
    template <typename E>
    static constexpr AddressType elem_align() {
        if constexpr (is_bundle_v<E>) {
            return E::alignment();
        } else {
            return alignof(E);
        }
    }
    
    template <typename T>
    static constexpr AddressType Location(uint32_t instance_id)
    {
        uint32_t page = instance_id / page_size;
        uint32_t offset = instance_id % page_size;
        return page * page_stride() + find_offset<T, 0, Es...>(offset);
    }

    static constexpr AddressType page_stride()
    {
        return align_up<page_stride_helper<0, Es...>(), 256>;
    }

#ifdef __CPPSL__
    template <typename T>
    static T Load(const data_layout::ByteAddressBuffer& buffer, uint32_t instance_id)
    {
        const auto offset = PagedLayout::Location<T>(instance_id);
        return buffer.Load<T>(offset);
    }
#endif

private:
    template <AddressType Off, typename E, typename... Rs>
    static constexpr AddressType page_stride_helper() {
        constexpr auto aligned = align_up<Off, elem_align<E>()>;
        constexpr auto next = aligned + elem_size<E>() * page_size;
        if constexpr (sizeof...(Rs) == 0) {
            return next;
        } else {
            return page_stride_helper<next, Rs...>();
        }
    }
public:

    template <typename Cb>
    static void for_each_component(Cb cb)
    {
        for_each_impl<0, Es...>(cb, 0);
    }
    
    static constexpr uint32_t get_component_count()
    {
        return ((is_bundle_v<Es> ? Es::component_count : 1) + ... + 0);
    }
    
private:
    template <uint32_t ID, typename E, typename... Rs, typename Cb>
    static void for_each_impl(Cb cb, uint32_t idx)
    {
        if constexpr (is_bundle_v<E>)
        {
            []<typename... Bs>(ComponentBundle<Bs...>, auto cb, uint32_t id, uint32_t idx) {
                uint32_t i = id, j = idx;
                ((cb(i++, sizeof(Bs), alignof(Bs), j++), ...));
            }(E{}, cb, ID, idx);
            if constexpr (sizeof...(Rs) > 0)
                for_each_impl<ID + E::component_count, Rs...>(cb, idx + E::component_count);
        }
        else
        {
            cb(ID, sizeof(E), alignof(E), idx);
            if constexpr (sizeof...(Rs) > 0)
                for_each_impl<ID + 1, Rs...>(cb, idx + 1);
        }
    }
    
private:
    template <typename T, AddressType Off, typename E, typename... Rs>
    static constexpr AddressType find_offset(uint32_t inst_off)
    {
        constexpr auto aligned = align_up<Off, elem_align<E>()>;
        if constexpr (is_bundle_v<E>)
        {
            if constexpr (E::template contains<T>())
                return aligned + inst_off * E::size() + E::template offset_in_bundle<T>();
        }
        else if constexpr (std::is_same_v<T, E>)
        {
            return aligned + inst_off * sizeof(E);
        }
        if constexpr (sizeof...(Rs) > 0)
            return find_offset<T, aligned + elem_size<E>() * page_size, Rs...>(inst_off);
        return AddressType(-1);
    }
};

// #define BUNDLE(...) skr::ComponentBundle<__VA_ARGS__>

// 连续布局（需要 capacity）
/*
template <typename... Es>
struct ContinuousLayout
{
    template <typename... T> struct TypePack {};
    using Elements = TypePack<Es...>;
    uint32_t capacity;
    
    constexpr ContinuousLayout(uint32_t cap) : capacity(cap) {}
    
    template <typename E>
    static constexpr AddressType elem_size() {
        if constexpr (is_bundle_v<E>) {
            return E::size();
        } else {
            return sizeof(E);
        }
    }
    
    template <typename E>
    static constexpr AddressType elem_align() {
        if constexpr (is_bundle_v<E>) {
            return E::alignment();
        } else {
            return alignof(E);
        }
    }
    
    template <typename T>
    AddressType location(uint32_t id) const {
        return find<T, 0, Es...>(id);
    }
    
    template <typename Cb>
    static void for_each_component(Cb cb) {
        for_each_impl<0, Es...>(cb, 0);
    }
    
    static constexpr uint32_t get_component_count() {
        return ((is_bundle_v<Es> ? Es::component_count : 1) + ... + 0);
    }
    
private:
    template <typename T, AddressType O, typename E, typename... R>
    AddressType find(uint32_t id) const {
        constexpr auto a = align_up<O, elem_align<E>()>;
        if constexpr (is_bundle_v<E>) {
            if constexpr (E::template contains<T>())
                return a + id * E::size() + E::template offset_in_bundle<T>();
        } else if constexpr (std::is_same_v<T, E>) {
            return a + id * sizeof(E);
        }
        if constexpr (sizeof...(R) > 0)
            return find<T, a + elem_size<E>() * capacity, R...>(id);
        return AddressType(-1);
    }
    
    template <uint32_t ID, typename E, typename... Rs, typename Cb>
    static void for_each_impl(Cb cb, uint32_t idx) {
        if constexpr (is_bundle_v<E>) {
            []<typename... Bs>(ComponentBundle<Bs...>, auto cb, uint32_t id, uint32_t idx) {
                uint32_t i = id, j = idx;
                ((cb(i++, sizeof(Bs), alignof(Bs), j++), ...));
            }(E{}, cb, ID, idx);
            if constexpr (sizeof...(Rs) > 0)
                for_each_impl<ID + E::component_count, Rs...>(cb, idx + E::component_count);
        } else {
            cb(ID, sizeof(E), alignof(E), idx);
            if constexpr (sizeof...(Rs) > 0)
                for_each_impl<ID + 1, Rs...>(cb, idx + 1);
        }
    }
};
*/

} // namespace skr