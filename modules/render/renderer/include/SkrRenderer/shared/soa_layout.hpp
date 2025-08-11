#pragma once
#ifdef __CPPSL__
#include "detail/type_usings.hppsl"
#else
#include "detail/type_usings.hpp"
#endif

namespace skr::renderer
{

using uint32_t = skr::data_layout::uint32_t;
using AddressType = skr::data_layout::AddressType;

// 组件束：将多个组件打包在一起（AOS 布局）
template<typename... BundledComponents>
struct ComponentBundle {
    static constexpr size_t component_count = sizeof...(BundledComponents);
    
    // Bundle 的总大小（所有组件紧密排列）
    static constexpr AddressType size() {
        AddressType total = 0;
        ((total = align_up(total, alignof(BundledComponents)) + sizeof(BundledComponents)), ...);
        return total;
    }
    
    // Bundle 的对齐要求（取最大值）
    static constexpr AddressType alignment() {
        AddressType max_align = 1;
        ((max_align = (alignof(BundledComponents) > max_align) ? alignof(BundledComponents) : max_align), ...);
        return max_align;
    }
    
    // 组件在 Bundle 内的偏移
    template<typename T>
    static constexpr AddressType offset_in_bundle() {
        return calculate_offset<T, 0, BundledComponents...>();
    }
    
    // 检查类型是否在 Bundle 中
    template<typename T>
    static constexpr bool contains() {
        return ((std::is_same_v<T, BundledComponents>) || ...);
    }
    
private:
    template<typename Target, AddressType Offset, typename First, typename... Rest>
    static constexpr AddressType calculate_offset() {
        constexpr AddressType aligned = align_up(Offset, alignof(First));
        if constexpr (std::is_same_v<Target, First>) {
            return aligned;
        } else if constexpr (sizeof...(Rest) > 0) {
            return calculate_offset<Target, aligned + sizeof(First), Rest...>();
        } else {
            return AddressType(-1);
        }
    }
    
    static constexpr AddressType align_up(AddressType value, AddressType alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }
};

// 类型特征：检测是否是 ComponentBundle
template<typename T>
struct is_bundle : std::false_type {};

template<typename... Ts>
struct is_bundle<ComponentBundle<Ts...>> : std::true_type {};

template<typename T>
inline constexpr bool is_bundle_v = is_bundle<T>::value;

// 统一的分页布局：支持 SOA 和 AOSOA
template<uint32_t PageSize = 16384, typename... Elements>
struct PagedLayout 
{
public:
    static constexpr AddressType element_count = sizeof...(Elements);
    static constexpr uint32_t page_size = PageSize;
    
    // 获取组件位置（自动处理 Bundle 和独立组件）
    template<typename T>
    inline static constexpr AddressType component_location(uint32_t instance_id) {
        uint32_t page = instance_id / page_size;
        uint32_t offset = instance_id % page_size;
        AddressType page_start = page * page_size_in_bytes();
        return page_start + find_component_offset<T, 0, Elements...>(offset);
    }
    
    inline static constexpr AddressType page_size_in_bytes() {
        AddressType total = 0;
        ((total = align_up(total, element_alignment<Elements>()) + element_size<Elements>() * page_size), ...);
        return align_up(total, 256);
    }
    
    inline static constexpr AddressType total_size_in_bytes(uint32_t instance_count) {
        uint32_t page_count = (instance_count + page_size - 1) / page_size;
        return page_size_in_bytes() * page_count;
    }
    
private:
    // 获取元素大小（Bundle 或单个组件）
    template<typename E>
    inline static constexpr AddressType element_size() {
        if constexpr (is_bundle_v<E>) {
            return E::size();
        } else {
            return sizeof(E);
        }
    }
    
    // 获取元素对齐（Bundle 或单个组件）
    template<typename E>
    inline static constexpr AddressType element_alignment() {
        if constexpr (is_bundle_v<E>) {
            return E::alignment();
        } else {
            return alignof(E);
        }
    }
    
    // 查找组件偏移
    template<typename Target, AddressType Offset, typename First, typename... Rest>
    inline static constexpr AddressType find_component_offset(uint32_t instance_offset) {
        constexpr AddressType aligned = align_up(Offset, element_alignment<First>());
        
        if constexpr (is_bundle_v<First>) {
            // Bundle：检查目标是否在其中
            if constexpr (First::template contains<Target>()) {
                constexpr AddressType bundle_offset = First::template offset_in_bundle<Target>();
                return aligned + instance_offset * First::size() + bundle_offset;
            }
        } else if constexpr (std::is_same_v<Target, First>) {
            // 独立组件
            return aligned + instance_offset * sizeof(First);
        }
        
        // 继续查找
        if constexpr (sizeof...(Rest) > 0) {
            constexpr AddressType next = aligned + element_size<First>() * page_size;
            return find_component_offset<Target, next, Rest...>(instance_offset);
        } else {
            return AddressType(-1);
        }
    }
    
    inline static constexpr AddressType align_up(AddressType value, AddressType alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }
};

// 向后兼容的别名
template<uint32_t PageSize = 16384, typename... Components>
using PagedSOALayout = PagedLayout<PageSize, Components...>;

// 便捷宏
#define BUNDLE(...) skr::renderer::ComponentBundle<__VA_ARGS__>

#define DECLARE_PAGED_LAYOUT(name, ...) \
    using name = skr::renderer::PagedLayout<16384, __VA_ARGS__>

#define DECLARE_PAGED_LAYOUT_WITH_PAGE_SIZE(name, page_size, ...) \
    using name = skr::renderer::PagedLayout<page_size, __VA_ARGS__>

// 向后兼容
#define DECLARE_PAGED_SOA(name, ...) \
    using name = skr::renderer::PagedLayout<16384, __VA_ARGS__>

#define DECLARE_PAGED_SOA_WITH_PAGE_SIZE(name, page_size, ...) \
    using name = skr::renderer::PagedLayout<page_size, __VA_ARGS__>

} // namespace skr::renderer