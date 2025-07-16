# SakuraEngine 容器使用指南

## 概述

SakuraEngine 使用自定义的容器库，提供了类似 STL 的功能但有不同的 API 约定。本文档介绍引擎容器的正确使用方式。

## 核心容器类型

### skr::Vector

动态数组容器，类似于 `std::vector`。

```cpp
#include "SkrContainers/vector.hpp"

skr::Vector<int> numbers;
skr::Vector<MyClass*> pointers;
```

### skr::Map

基于稀疏哈希表的键值映射容器，提供高性能的查找和插入操作。

```cpp
#include "SkrContainers/map.hpp"

skr::Map<uint32_t, SystemWindow*> window_cache;
skr::Map<HWND, bool> tracking_state;
```

#### Map API 说明

| 操作 | API | 说明 |
|------|-----|------|
| 添加/更新 | `add(key, value)` | 插入新键值对或更新已存在的值 |
| 查找 | `find(key)` | 返回 DataRef，使用 `value()` 方法获取值 |
| 检查存在 | `contains(key)` | 返回 bool，检查键是否存在 |
| 删除 | `remove(key)` | 删除指定键的键值对，返回是否成功 |
| 清空 | `clear()` | 清空所有键值对 |
| 大小 | `size()` | 返回键值对数量 |
| 判空 | `is_empty()` | 检查是否为空 |

#### Map 使用示例

```cpp
// 基本操作
skr::Map<int, std::string> map;

// 添加或更新
map.add(1, "one");
map.add(2, "two");
map.add(1, "ONE");  // 更新键1的值

// 检查存在
if (map.contains(1))
{
    // 键存在
}

// 查找并使用值
if (auto ref = map.find(1))
{
    auto& value = ref.value();  // 获取值的引用
    // 使用 value
}

// 删除
bool removed = map.remove(2);  // 返回 true 如果删除成功

// 遍历（使用范围 for）
for (const auto& [key, value] : map)
{
    // 处理键值对
}
```

## API 差异对照表

| STL 方法 | SakuraEngine 方法 | 说明 |
|----------|-------------------|------|
| `push_back()` | `add()` | 添加元素到末尾 |
| `emplace_back()` | `emplace()` | 就地构造元素 |
| `clear()` | `clear()` | 清空容器（保持不变） |
| `empty()` | `is_empty()` | 检查是否为空 |
| `find()` | `find()` | 查找元素（返回值不同） |

## 关键使用模式

### 1. 添加元素

```cpp
// 错误：使用 STL 风格
vector.push_back(element);
vector.emplace_back(args...);

// 正确：使用引擎风格
vector.add(element);
vector.emplace(args...);
```

### 2. 查找元素

引擎的 `find()` 方法返回一个特殊的引用类型，而不是迭代器。

#### Vector 的 find 方法

```cpp
// Vector::find 返回 DataRef/CDataRef
auto ref = vector.find(value);
if (ref)  // 使用 operator bool() 检查是否找到
{
    size_t index = ref.index();     // 获取索引
    auto* ptr = ref.ptr();          // 获取指针
    auto& element = ref.ref();      // 获取引用
}

// 完整示例
bool remove_element(skr::Vector<T>& vec, const T& value)
{
    if (auto ref = vec.find(value))
    {
        vec.remove_at(ref.index());
        return true;
    }
    return false;
}
```

#### Map 的 find 方法

```cpp
// Map::find 返回包含 value() 方法的引用类型
if (auto found = map.find(key))
{
    auto* value = found.value();  // 获取值指针
    // 使用 value
}
```

### 3. 清空容器

```cpp
// 两种容器都使用 clear()
vector.clear();
map.clear();
```

### 4. 检查空容器

```cpp
// 错误：使用 empty()
if (vector.empty()) { }

// 正确：使用 is_empty()
if (vector.is_empty()) { }
```

## 实际代码示例

### 事件队列实现

```cpp
struct SystemEventQueue::Impl
{
    skr::Vector<ISystemEventSource*> sources;
    skr::Vector<ISystemEventHandler*> handlers;
};

bool SystemEventQueue::add_source(ISystemEventSource* source)
{
    if (!pimpl || !source)
        return false;
        
    // 使用 find 检查重复
    if (pimpl->sources.find(source))
        return false;
    
    // 使用 add 添加元素
    pimpl->sources.add(source);
    return true;
}

bool SystemEventQueue::remove_source(ISystemEventSource* source)
{
    if (!pimpl || !source)
        return false;
        
    // 使用 find 定位元素
    if (auto ref = pimpl->sources.find(source))
    {
        // 使用 index() 获取位置并删除
        pimpl->sources.remove_at(ref.index());
        return true;
    }
    return false;
}
```

### IME 候选词处理

```cpp
void SDL3IME::update_candidate_state(const SDL_TextEditingCandidatesEvent& event)
{
    candidate_state_.candidates.clear();
    candidate_state_.candidates.reserve(event.num_candidates);
    
    for (int32_t i = 0; i < event.num_candidates; ++i)
    {
        // 使用 emplace 而不是 emplace_back
        candidate_state_.candidates.emplace((const char8_t*)event.candidates[i]);
    }
}
```

### 窗口缓存管理

```cpp
class SDL3SystemApp : public SystemApp
{
private:
    skr::Map<SDL_WindowID, SDL3Window*> window_cache;
    skr::Vector<SDL3Monitor*> monitor_list;
    
    void refresh_monitors()
    {
        // 清空现有数据
        monitor_cache.clear();
        monitor_list.clear();
        
        // 添加新数据
        for (int i = 0; i < count; ++i)
        {
            auto* monitor = SkrNew<SDL3Monitor>(displays[i]);
            monitor_cache.add(displays[i], monitor);
            monitor_list.add(monitor);  // 使用 add 而不是 push_back
        }
    }
};
```

### 鼠标跟踪状态管理

```cpp
class Win32EventSource : public ISystemEventSource
{
private:
    // 使用 Map 跟踪每个窗口的鼠标进入状态
    skr::Map<HWND, bool> mouse_tracking_;
    
    bool poll_event(SkrSystemEvent& event)
    {
        MSG msg;
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_NOREMOVE))
        {
            if (msg.message == WM_MOUSEMOVE && msg.hwnd)
            {
                // 使用 contains 检查是否已跟踪
                if (!mouse_tracking_.contains(msg.hwnd))
                {
                    // 使用 add 添加跟踪状态
                    mouse_tracking_.add(msg.hwnd, true);
                    
                    // 注册鼠标离开通知
                    TRACKMOUSEEVENT tme = {};
                    tme.cbSize = sizeof(tme);
                    tme.dwFlags = TME_LEAVE;
                    tme.hwndTrack = msg.hwnd;
                    TrackMouseEvent(&tme);
                    
                    // 生成鼠标进入事件
                    // ...
                }
            }
        }
        // ...
    }
    
    void handle_mouse_leave(HWND hwnd)
    {
        // 使用 remove 清除跟踪状态
        mouse_tracking_.remove(hwnd);
        
        // 生成鼠标离开事件
        // ...
    }
};
```

## DataRef 详解

`DataRef` 是引擎容器查找操作返回的特殊类型，提供了安全和便捷的元素访问。

```cpp
template <typename T, typename TSize, bool kConst>
struct VectorDataRef {
    // 检查是否有效（找到元素）
    bool is_valid() const;
    explicit operator bool() const;
    
    // 访问元素
    DataType* ptr() const;      // 获取指针
    DataType& ref() const;      // 获取引用
    SizeType index() const;     // 获取索引
    
    // 安全访问
    DataType value_or(DataType v) const;        // 返回值或默认值
    DataType* pointer_or(DataType* p) const;    // 返回指针或默认指针
};
```

## 最佳实践

### DO - 推荐做法

1. **使用正确的方法名**
   - 添加元素用 `add()` 而不是 `push_back()`
   - 检查空容器用 `is_empty()` 而不是 `empty()`

2. **利用 find 返回的 DataRef**
   ```cpp
   if (auto ref = container.find(value))
   {
       // 使用 ref.index(), ref.ptr() 等
   }
   ```

3. **预分配容量**
   ```cpp
   vector.reserve(expected_size);
   ```

### DON'T - 避免做法

1. **不要假设 STL 接口存在**
   ```cpp
   // 错误：假设有 STL 风格的方法
   vector.push_back(item);
   vector.emplace_back(args...);
   
   // 正确：使用引擎的方法
   vector.add(item);
   vector.emplace(args...);
   ```

2. **不要使用迭代器查找**
   ```cpp
   // 错误：手动迭代
   for (size_t i = 0; i < vector.size(); ++i)
   {
       if (vector[i] == target)
       {
           vector.remove_at(i);
           break;
       }
   }
   
   // 正确：使用 find
   if (auto ref = vector.find(target))
   {
       vector.remove_at(ref.index());
   }
   ```

## 总结

SakuraEngine 的容器库提供了高性能、类型安全的容器实现。虽然 API 与 STL 有所不同，但通过遵循本文档的指导原则，可以充分发挥其优势。关键是记住主要的 API 差异，特别是 `add()` vs `push_back()`，以及 `find()` 返回的 `DataRef` 类型的使用方式。