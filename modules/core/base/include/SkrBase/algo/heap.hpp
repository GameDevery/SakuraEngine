#pragma once
#include "SkrBase/config.h"
#include "utils.hpp"
#include "SkrBase/misc/swap.hpp"

namespace skr::algo
{
// heap jump
template <typename T>
SKR_INLINE constexpr T heap_lchild_idx(T index) { return index * 2 + 1; }
template <typename T>
SKR_INLINE constexpr T heap_rchild_idx(T index) { return heap_lchild_idx(index) + 1; }
template <typename T>
SKR_INLINE constexpr T heap_parent_idx(T index) { return index ? (index - 1) / 2 : 0; }
template <typename T>
SKR_INLINE constexpr bool heap_is_leaf(T index, T count) { return heap_lchild_idx(index) >= count; }

// sift down
template <typename T, typename TSize, typename TP = Less<>>
SKR_INLINE void heap_sift_down(T heap, TSize idx, TSize count, TP&& p = TP())
{
    using Swapper = Swap<std::decay_t<decltype(*heap)>>;

    while (!heap_is_leaf(idx, count))
    {
        const TSize l_child_idx = heap_lchild_idx(idx);
        const TSize r_child_idx = l_child_idx + 1;

        // find min child
        TSize min_child_idx = l_child_idx;
        if (r_child_idx < count)
        {
            min_child_idx = p(*(heap + l_child_idx), *(heap + r_child_idx)) ? l_child_idx : r_child_idx;
        }

        // now element is on his location
        if (!p(*(heap + min_child_idx), *(heap + idx)))
            break;

        Swapper::call(*(heap + idx), *(heap + min_child_idx));
        idx = min_child_idx;
    }
}

// sift up
template <class T, typename TSize, class TP = Less<>>
SKR_INLINE TSize heap_sift_up(T* heap, TSize root_idx, TSize node_idx, TP&& p = TP())
{
    using Swapper = Swap<std::decay_t<decltype(*heap)>>;

    while (node_idx > root_idx)
    {
        TSize parent_idx = heap_parent_idx(node_idx);

        // now element is on his location
        if (!p(*(heap + node_idx), *(heap + parent_idx)))
            break;

        Swapper::call(*(heap + node_idx), *(heap + parent_idx));
        node_idx = parent_idx;
    }

    return node_idx;
}

// is heap
template <typename T, typename TSize, typename TP = Less<>>
SKR_INLINE bool is_heap(T* heap, TSize count, TP&& p = TP())
{
    for (TSize i = 1; i < count; ++i)
    {
        if (p(*(heap + i), *(heap + heap_parent_idx(i))))
            return false;
    }
    return true;
}

// heapify
template <typename T, typename TSize, typename TP = Less<>>
SKR_INLINE void heapify(T* heap, TSize count, TP&& p = TP())
{
    if (count > 1)
    {
        TSize idx = heap_parent_idx(count - 1);
        while (true)
        {
            heap_sift_down(heap, idx, count, std::forward<TP>(p));
            if (idx == 0)
                return;
            --idx;
        }
    }
}

// heap sort
template <typename T, typename TSize, class TP = Less<>>
SKR_INLINE void heap_sort(T heap, TSize count, TP&& p = TP())
{
    using Swapper = Swap<std::decay_t<decltype(*heap)>>;

    if (count == 0) return;
    auto reverse_pred = [&](const auto& a, const auto& b) -> bool { return !p(a, b); };

    // use reverse_pred heapify, and pop head swap to tail
    heapify(heap, count, reverse_pred);

    for (TSize cur_count = count - 1; cur_count > 0; --cur_count)
    {
        Swapper::call(*heap, *(heap + cur_count));
        heap_sift_down(heap, (TSize)0, cur_count, reverse_pred);
    }
}
} // namespace skr::algo