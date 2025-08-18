# Win32 Backend Implementation

## 概述

Win32 后端为 SakuraEngine 的 Window System 提供了原生 Windows 支持，直接使用 Windows API 而非 SDL3，可以获得更好的平台集成和性能。

## 实现特性

### 核心功能

1. **窗口管理**
   - 支持所有窗口样式（普通、无边框、置顶、工具提示）
   - 窗口大小调整、移动、最大化/最小化
   - 全屏模式切换
   - 透明度控制
   - DPI 感知（Per-Monitor DPI）

2. **显示器管理**
   - 多显示器枚举和信息查询
   - 显示模式列表
   - DPI 缩放信息
   - 主显示器识别

3. **事件系统**
   - 完整的键盘、鼠标事件
   - 窗口事件（大小改变、移动、焦点等）
   - 与 SystemEventQueue 集成
   - 线程安全的事件队列

4. **IME 支持**
   - 使用 Windows IMM32 API
   - 组合字符串实时更新
   - 候选词列表
   - 输入位置控制

## 架构设计

### 类层次结构

```
Win32SystemApp : SystemApp
├── Win32Window : SystemWindow
├── Win32Monitor : SystemMonitor  
├── Win32IME : IME
└── Win32EventSource : ISystemEventSource
```

### 关键实现细节

#### 1. 窗口过程

使用统一的窗口过程处理所有窗口消息：

```cpp
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // 1. 获取窗口对象指针
    Win32Window* window = reinterpret_cast<Win32Window*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA)
    );
    
    // 2. 让窗口处理消息
    if (window)
    {
        LRESULT result = window->handle_message(msg, wParam, lParam);
        if (result != -1)
            return result;
    }
    
    // 3. 默认处理
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
```

#### 2. DPI 感知

支持 Windows 8.1+ 的 Per-Monitor DPI：

```cpp
// 初始化时设置 DPI 感知
if (IsWindows8Point1OrGreater())
{
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
}
else
{
    SetProcessDPIAware();
}

// 获取显示器 DPI
GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
```

#### 3. IME 集成

IME 实现使用 IMM32 API，处理以下消息：

- `WM_IME_STARTCOMPOSITION` - 开始输入
- `WM_IME_COMPOSITION` - 组合更新
- `WM_IME_ENDCOMPOSITION` - 结束输入
- `WM_IME_NOTIFY` - 候选词通知

#### 4. 事件转换

将 Windows 消息转换为引擎事件：

```cpp
bool process_message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, SkrSystemEvent& out_event)
{
    switch (msg)
    {
        case WM_KEYDOWN:
            out_event.type = SKR_SYSTEM_EVENT_KEY_DOWN;
            out_event.key.keycode = translate_vk_to_keycode(wParam);
            // ...
    }
}
```

## 与 SDL3 后端的对比

| 特性 | Win32 | SDL3 |
|------|-------|------|
| 平台支持 | 仅 Windows | 跨平台 |
| 依赖 | Windows SDK | SDL3 库 |
| 性能 | 略优（直接调用） | 很好 |
| 功能完整性 | 平台特定功能更多 | 通用功能 |
| 维护成本 | 需要维护平台代码 | SDL 团队维护 |
| 二进制大小 | 更小 | 需要 SDL3.dll |

## 使用示例

```cpp
// 创建 Win32 后端
SystemApp* app = SystemApp::Create("Win32");

// 创建窗口
SystemWindowCreateInfo info;
info.title = u8"Win32 Window";
info.size = {1280, 720};
SystemWindow* window = app->create_window(info);

// 使用 IME
IME* ime = app->get_ime();
ime->start_text_input(window);

// 事件循环
while (running)
{
    app->get_event_queue()->pump_messages();
    // 渲染...
}
```

## 平台特定功能

### 1. 获取 HWND

```cpp
HWND hwnd = static_cast<HWND>(window->get_native_handle());
```

### 2. 消息钩子

可以在 `Win32Window::handle_message` 中添加自定义消息处理。

### 3. 扩展窗口样式

通过修改 `create_window` 中的样式标志支持更多 Windows 特性。

## 已知限制

1. **触摸输入** - 尚未实现 WM_TOUCH 消息处理
2. **原始输入** - 未使用 Raw Input API
3. **高精度定时器** - 使用 GetTickCount64 而非 QueryPerformanceCounter

## 调试建议

1. **使用 Spy++** 查看窗口消息
2. **检查 GetLastError()** 获取详细错误
3. **启用 IME 调试日志**：
   ```cpp
   ImmSetOpenStatus(hIMC, TRUE);  // 强制打开 IME
   ```

## 总结

Win32 后端提供了原生 Windows 体验，特别适合：

- 需要深度 Windows 集成的应用
- 不需要跨平台的项目
- 追求最小依赖的场景

对于跨平台项目，建议使用 SDL3 后端。