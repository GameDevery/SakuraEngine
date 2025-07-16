# SakuraEngine Window System

## 概述

SakuraEngine 的 Window System 提供了跨平台的窗口管理、显示器检测和输入法编辑器（IME）支持。系统采用简洁的抽象设计，避免过度工程，专注于游戏引擎的实际需求。

## 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                      SystemApp                               │
│                   (具体管理类)                               │
│  ┌───────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │ Event Queue   │  │ Window Manager  │  │     IME      │ │
│  └───────────────┘  └─────────────────┘  └──────────────┘ │
└─────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┴─────────────────────┐
        │                                           │
┌───────────────────┐                       ┌──────────────┐
│  SDL3 Backend     │                       │ Win32 Backend│
│  ┌─────────────┐  │                       │  ┌─────────┐ │
│  │EventSource  │  │                       │  │Event    │ │
│  │WindowManager│  │                       │  │Source   │ │
│  │IME          │  │                       │  │Window   │ │
│  └─────────────┘  │                       │  │Manager  │ │
│   (已实现)        │                       │  │IME      │ │
└───────────────────┘                       │  └─────────┘ │
                                            │   (已实现)   │
                                            └──────────────┘
```

## 核心组件

### SystemApp

系统应用程序的主管理类，负责初始化和协调各个子系统。

```cpp
struct SystemApp {
    // 子系统访问
    IME* get_ime() const { return ime; }
    ISystemWindowManager* get_window_manager() const { return window_manager; }
    SystemEventQueue* get_event_queue() const { return event_queue; }
    
    // 事件源管理
    bool add_event_source(ISystemEventSource* source);
    bool remove_event_source(ISystemEventSource* source);
    ISystemEventSource* get_platform_event_source() const { return platform_event_source; }
    
    // 阻塞等待事件
    bool wait_events(uint32_t timeout_ms = 0);
    
    // 工厂方法
    static SystemApp* Create(const char* backend = nullptr);
    static void Destroy(SystemApp* app);
};
```

### ISystemWindowManager

窗口管理器基类，提供窗口和显示器管理的通用功能。

```cpp
struct ISystemWindowManager {
    // 窗口管理 - 基类实现
    SystemWindow* create_window(const SystemWindowCreateInfo& info) SKR_NOEXCEPT;
    void destroy_window(SystemWindow* window) SKR_NOEXCEPT;
    void destroy_all_windows() SKR_NOEXCEPT;
    
    // 窗口查找
    SystemWindow* get_window_by_native_handle(void* native_handle) const SKR_NOEXCEPT;
    SystemWindow* get_window_from_event(const SkrSystemEvent& event) const SKR_NOEXCEPT;
    
    // 窗口枚举
    void get_all_windows(skr::Vector<SystemWindow*>& out_windows) const SKR_NOEXCEPT;
    uint32_t get_window_count() const SKR_NOEXCEPT;
    
    // 显示器管理 - 必须由平台实现
    virtual uint32_t get_monitor_count() const = 0;
    virtual SystemMonitor* get_monitor(uint32_t index) const = 0;
    virtual SystemMonitor* get_primary_monitor() const = 0;
    virtual SystemMonitor* get_monitor_from_point(int32_t x, int32_t y) const = 0;
    virtual SystemMonitor* get_monitor_from_window(SystemWindow* window) const = 0;
    virtual void refresh_monitors() = 0;
    
    // 显示器枚举 - 基类提供默认实现
    virtual void enumerate_monitors(void (*callback)(SystemMonitor* monitor, void* user_data), void* user_data) const;
    
    // 工厂方法
    static ISystemWindowManager* Create(const char* backend = nullptr);
    static void Destroy(ISystemWindowManager* manager);
    
protected:
    // 平台必须实现的窗口创建/销毁
    virtual SystemWindow* create_window_internal(const SystemWindowCreateInfo& info) = 0;
    virtual void destroy_window_internal(SystemWindow* window) = 0;
};
```

### SystemWindow

窗口抽象接口，提供基本的窗口操作。

```cpp
struct SystemWindow {
    // 属性管理
    virtual void set_title(const skr::String& title) = 0;
    virtual void set_position(int32_t x, int32_t y) = 0;
    virtual void set_size(uint32_t width, uint32_t height) = 0;
    
    // 状态控制
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void minimize() = 0;
    virtual void maximize() = 0;
    virtual void focus() = 0;
    
    // 全屏支持
    virtual void set_fullscreen(bool fullscreen, SystemMonitor* monitor = nullptr) = 0;
    
    // 平台句柄
    virtual void* get_native_handle() const = 0;
};
```

### SystemMonitor

显示器信息和管理。

```cpp
struct SystemMonitor {
    virtual SystemMonitorInfo get_info() const = 0;
    virtual uint32_t get_display_mode_count() const = 0;
    virtual SystemMonitorDisplayMode get_display_mode(uint32_t index) const = 0;
    virtual SystemMonitorDisplayMode get_current_display_mode() const = 0;
};
```

### IME (Input Method Editor)

输入法编辑器支持，用于处理复杂文本输入（如中日韩文字）。

```cpp
class IME {
    // 会话管理
    virtual void start_text_input(SystemWindow* window) = 0;
    virtual void stop_text_input() = 0;
    
    // 输入区域设置
    virtual void set_text_input_area(const IMETextInputArea& area) = 0;
    
    // 事件回调
    virtual void set_event_callbacks(const IMEEventCallbacks& callbacks) = 0;
    
    // 状态查询
    virtual IMECompositionInfo get_composition_info() const = 0;
    virtual IMECandidateInfo get_candidates_info() const = 0;
};
```

## 设计理念

### 1. 简洁性优先

- **避免过度抽象**：只暴露游戏引擎真正需要的功能
- **清晰的职责**：每个组件有明确的单一职责
- **最小化依赖**：组件间保持松耦合

### 2. 平台无关性

- **纯虚接口**：所有平台相关实现隐藏在后端
- **统一的行为**：不同平台提供一致的行为保证
- **优雅降级**：平台不支持的功能安全地返回默认值

### 3. 鲁棒性设计

- **防御性编程**：所有输入参数验证
- **早期返回**：减少嵌套，提高可读性
- **明确的错误处理**：使用日志记录而非异常

## 实现要点

### SDL3 后端实现

当前实现基于 SDL3，提供了完整的跨平台支持。

#### 关键特性

1. **自动 DPI 感知**：SDL3 自动处理高 DPI 显示
2. **事件驱动 IME**：通过 SDL 事件系统处理输入法
3. **智能资源管理**：使用 RAII 和智能指针

#### 实现细节

```cpp
// 窗口创建示例
SystemWindow* SDL3SystemApp::create_window(const SystemWindowCreateInfo& info) {
    // 参数验证
    if (info.size.x == 0 || info.size.y == 0) {
        SKR_LOG_ERROR(u8"Invalid window size");
        return nullptr;
    }
    
    // 创建 SDL 窗口
    SDL_Window* sdl_window = SDL_CreateWindow(...);
    if (!sdl_window) {
        SKR_LOG_ERROR(u8"Failed to create window: %s", SDL_GetError());
        return nullptr;
    }
    
    // 包装并缓存
    auto* window = SkrNew<SDL3Window>(sdl_window);
    window_cache.add(SDL_GetWindowID(sdl_window), window);
    return window;
}
```

### IME 系统设计

IME 系统采用事件驱动设计，支持组合文本（composition）和候选词（candidates）。

#### 核心状态

1. **CompositionState**：跟踪正在输入的文本
2. **CandidateState**：管理候选词列表
3. **输入区域**：告诉 IME 在哪里显示候选窗口

#### 简化策略

- 移除了不必要的转换模式（ConversionMode）
- 依赖 SDL3 自动管理屏幕键盘
- 不实现平台特定的高级功能

## 事件系统集成

Window System 现在集成了统一的事件处理系统，提供了灵活的事件源和处理器架构。

### 事件系统架构

```
┌─────────────────────────────────────────────────┐
│              SystemEventQueue                    │
│  ┌─────────────────┐  ┌────────────────────┐   │
│  │ Event Sources   │  │  Event Handlers    │   │
│  │                 │  │                    │   │
│  │ - SDL3Source    │  │ - User Handlers   │   │
│  │ - Win32Source   │  │ - System Handlers │   │
│  │ - CustomSource  │  │                    │   │
│  └─────────────────┘  └────────────────────┘   │
└─────────────────────────────────────────────────┘
```

### ISystemEventSource

事件源接口，支持轮询和阻塞等待两种模式。

```cpp
struct ISystemEventSource {
    virtual ~ISystemEventSource() SKR_NOEXCEPT;
    virtual bool poll_event(SkrSystemEvent& event) SKR_NOEXCEPT = 0;
    virtual bool wait_events(uint32_t timeout_ms = 0) SKR_NOEXCEPT = 0; // 0 = 无限等待
};
```

### 关键特性

1. **统一事件处理**：所有系统事件（窗口、键盘、鼠标等）通过统一接口处理
2. **IME 事件协调**：IME 事件自动转发，通过回调机制处理
3. **可扩展架构**：支持添加自定义事件源和处理器
4. **非阻塞和阻塞模式**：支持轮询和等待两种事件处理模式

## 使用示例

### 创建窗口

```cpp
// 创建系统应用
auto* app = SystemApp::Create("SDL3"); // 或 "Win32"，nullptr 为自动检测

// 获取窗口管理器
auto* window_manager = app->get_window_manager();

// 创建窗口
SystemWindowCreateInfo info;
info.title = u8"My Game";
info.size = {1920, 1080};
info.is_resizable = true;

auto* window = window_manager->create_window(info);
window->show();
```

### 使用 IME

```cpp
// 获取 IME
auto* ime = app->get_ime();

// 设置回调
IMEEventCallbacks callbacks;
callbacks.on_text_input = [](const skr::String& text) {
    // 处理输入的文本
};
callbacks.on_composition_update = [](const IMECompositionInfo& info) {
    // 显示正在输入的文本
};

ime->set_event_callbacks(callbacks);

// 开始文本输入
ime->start_text_input(window);

// 设置输入位置（用于显示候选窗口）
IMETextInputArea area;
area.position = {100, 200};
area.size = {200, 30};
ime->set_text_input_area(area);
```

### 多显示器支持

```cpp
// 获取窗口管理器
auto* window_manager = app->get_window_manager();

// 枚举所有显示器
window_manager->enumerate_monitors([](SystemMonitor* monitor, void* userdata) {
    auto info = monitor->get_info();
    SKR_LOG_INFO(u8"Monitor: %s (%dx%d)", 
        info.name.c_str(), info.size.x, info.size.y);
}, nullptr);

// 在特定显示器上全屏
auto* primary = window_manager->get_primary_monitor();
window->set_fullscreen(true, primary);

// 获取窗口所在的显示器
auto* monitor = window_manager->get_monitor_from_window(window);
```

### 事件处理

```cpp
// 创建事件处理器
class MyEventHandler : public ISystemEventHandler {
    void handle_event(const SkrSystemEvent& event) override {
        switch (event.type) {
            case SKR_SYSTEM_EVENT_KEY_DOWN:
                // 处理按键
                break;
            case SKR_SYSTEM_EVENT_WINDOW_RESIZED:
                // 处理窗口大小变化
                break;
        }
    }
};

// 注册处理器
MyEventHandler handler;
app->get_event_queue()->add_handler(&handler);

// 事件循环
while (running) {
    // 非阻塞轮询
    app->get_event_queue()->pump_messages();
    
    // 或者阻塞等待（带超时）
    app->wait_events(100); // 100ms timeout
}

// 支持多事件源
CustomEventSource custom_source;
app->add_event_source(&custom_source);

// 稍后移除
app->remove_event_source(&custom_source);
```

## 最佳实践

### DO - 推荐做法

1. **验证输入参数**
   ```cpp
   if (!window) {
       SKR_LOG_ERROR(u8"Invalid window parameter");
       return;
   }
   ```

2. **使用 RAII 管理资源**
   ```cpp
   // 析构函数自动清理
   ~SDL3IME() {
       stop_text_input();
       if (text_input_props_) {
           SDL_DestroyProperties(text_input_props_);
       }
   }
   ```

3. **提供合理的默认值**
   ```cpp
   struct SystemWindowCreateInfo {
       skr::String title = u8"Untitled";
       uint2 size = {800, 600};
       // ...
   };
   ```

### DON'T - 避免做法

1. **避免过度设计**
   - 不要添加"可能有用"的功能
   - 不要创建深层继承体系

2. **不要忽略错误处理**
   - 总是检查返回值
   - 记录有意义的错误信息

3. **避免平台特定代码泄露**
   - 保持接口的平台无关性
   - 平台特定功能放在实现中

## 扩展指南

### 添加新的后端

1. 实现平台相关的组件
   ```cpp
   // 窗口管理器
   class CocoaWindowManager : public ISystemWindowManager {
       // 实现窗口和显示器管理
   };
   
   // 事件源
   class CocoaEventSource : public ISystemEventSource {
       // 实现事件轮询和等待
   };
   
   // IME
   class CocoaIME : public IME {
       // 实现输入法支持
   };
   ```

2. 创建工厂函数
   ```cpp
   // cocoa_system_factory.cpp
   ISystemEventSource* CreateCocoaEventSource(SystemApp* app);
   void ConnectCocoaComponents(ISystemWindowManager* window_manager, 
                              ISystemEventSource* event_source, 
                              IME* ime);
   ```

3. 在 SystemApp::initialize 中注册
   ```cpp
   if (strcmp(backend, "Cocoa") == 0) {
       window_manager = ISystemWindowManager::Create(backend);
       ime = IME::Create(backend);
       platform_event_source = CreateCocoaEventSource(this);
       ConnectCocoaComponents(window_manager, platform_event_source, ime);
   }
   ```

4. 注册到构建系统

### 添加新功能

1. 先在接口中添加纯虚方法
2. 在所有后端中实现（可以先返回默认值）
3. 逐步完善各平台实现

## 故障排除

### 常见问题

1. **IME 不工作**
   - 确认调用了 `start_text_input`
   - 检查是否设置了输入区域
   - 验证回调是否正确设置

2. **窗口创建失败**
   - 检查 SDL 错误信息
   - 验证窗口大小是否有效
   - 确认 SDL 初始化成功

3. **多显示器问题**
   - 调用 `refresh_monitors` 更新显示器信息
   - 处理显示器断开的情况

## 总结

SakuraEngine 的 Window System 提供了一个简洁、健壮的跨平台窗口管理解决方案。通过专注于游戏引擎的实际需求，避免过度设计，系统保持了良好的可维护性和扩展性。IME 支持确保了国际化文本输入的需求，而模块化的设计使得添加新平台支持变得简单直接。