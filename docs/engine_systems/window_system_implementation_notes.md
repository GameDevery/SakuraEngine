# Window System 实现经验总结

## 设计决策记录

### 1. 为什么选择 SDL3 作为首个后端

**决策**：使用 SDL3 而非直接调用平台 API

**理由**：
- SDL3 提供了成熟的跨平台抽象
- 内置 IME 支持，避免重复造轮子
- 自动处理 DPI 缩放等现代显示特性
- 游戏行业广泛使用，稳定可靠

**权衡**：
- 增加了依赖
- 某些平台特定功能可能受限
- 但整体收益大于成本

### 2. IME 系统简化

**原始设计**：包含转换模式、子句边界、屏幕键盘控制等功能

**简化后**：只保留核心功能
- 文本输入会话管理
- 组合文本跟踪
- 候选词列表
- 输入区域定位

**经验教训**：
- 过早的功能添加往往基于错误假设
- SDL3 已经处理了很多底层细节
- 简单的系统更容易维护和调试

### 3. 错误处理策略

**选择**：使用日志 + 返回值，而非异常

**实践**：
```cpp
if (!window) {
    SKR_LOG_ERROR(u8"Cannot start text input with null window");
    return;
}
```

**好处**：
- 与 C API 兼容
- 性能可预测
- 调试信息清晰

## 实现中的关键问题和解决方案

### 1. 线程安全

**问题**：IME 事件可能从不同线程触发

**解决方案**：
```cpp
mutable std::mutex state_mutex_;

void SDL3IME::process_event(const SDL_Event& event) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    // 处理事件
}
```

### 2. 生命周期管理

**问题**：Window 和 IME 的生命周期关系

**解决方案**：
- IME 由 SystemApp 拥有，生命周期独立于 Window
- Window 销毁时自动停止该窗口的输入会话
- 使用 RAII 确保资源清理

### 3. 平台差异处理

**问题**：不同平台的 IME 行为差异巨大

**解决方案**：
- 定义最小公共功能集
- 平台特定行为通过回调暴露
- 不试图统一所有平台行为

## 代码质量提升技巧

### 1. 使用辅助函数减少重复

```cpp
// 之前：多处重复的类型转换和空检查
SDL_Window* sdl_window = static_cast<SDL3Window*>(active_window_)->get_sdl_window();

// 之后：统一的辅助函数
SDL_Window* get_sdl_window(SystemWindow* window) const {
    if (!window) return nullptr;
    return static_cast<SDL3Window*>(window)->get_sdl_window();
}
```

### 2. 早期返回减少嵌套

```cpp
// 不好的风格
void function() {
    if (condition) {
        // 很多代码
        if (another_condition) {
            // 更多代码
        }
    }
}

// 好的风格
void function() {
    if (!condition) return;
    
    // 主要逻辑
    if (!another_condition) return;
    
    // 更多逻辑
}
```

### 3. 常量提取

```cpp
// 定义常量避免魔法数字
static constexpr int32_t kDefaultPageSize = 5;
static constexpr int32_t kInvalidIndex = -1;

// 使用
info.page_size = kDefaultPageSize;
```

## 性能考虑

### 1. 避免频繁的内存分配

```cpp
// 预留容量
candidate_state_.candidates.reserve(event.num_candidates);

// 使用 emplace_back
candidate_state_.candidates.emplace_back(
    (const char8_t*)event.candidates[i]
);
```

### 2. 缓存机制

```cpp
// 窗口 ID 到对象的映射，避免重复查找
skr::FlatHashMap<SDL_WindowID, SDL3Window*> window_cache;
```

### 3. 锁的粒度

- 使用 `mutable std::mutex` 允许 const 方法加锁
- 只在必要时加锁，减少锁的持有时间
- 考虑读写锁优化（如果读多写少）

## 调试技巧

### 1. 详细的日志

```cpp
SKR_LOG_INFO(u8"Refreshed monitors: found %d display(s)", count);
SKR_LOG_ERROR(u8"Failed to create SDL window: %s", SDL_GetError());
```

### 2. 状态验证

```cpp
// 在关键操作前验证状态
if (!active_window_) {
    SKR_LOG_WARN(u8"No active window for text input");
    return;
}
```

### 3. 资源追踪

- 在构造/析构函数中添加日志
- 使用智能指针自动管理生命周期
- 定期检查资源泄漏

## 未来改进方向

### 1. 事件系统集成

当前 IME 事件通过回调处理，未来可以：
- 集成到统一的事件系统
- 支持事件过滤和优先级
- 提供事件录制和回放

### 2. 更多后端支持

- Win32 原生实现（更好的 Windows 特性支持）
- Wayland 直接支持（Linux 现代显示服务器）
- Metal/Cocoa（macOS 原生功能）

### 3. 高级窗口特性

- 透明窗口
- 异形窗口
- 多窗口渲染目标共享

### 4. IME 增强

- 手写输入支持
- 语音输入集成
- 自定义候选词界面

## 经验总结

1. **从简单开始**：先实现核心功能，根据实际需求迭代
2. **依赖成熟方案**：SDL3 这样的库已经解决了很多问题
3. **防御性编程**：永远不要信任输入，做好错误处理
4. **保持一致性**：API 设计、命名、错误处理保持一致
5. **文档先行**：好的文档减少未来的维护成本

通过这个系统的实现，我们学到了在游戏引擎中设计跨平台子系统的重要原则：简洁、鲁棒、可扩展。这些经验可以应用到引擎的其他子系统设计中。