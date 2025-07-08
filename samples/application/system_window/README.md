# System Window Example

这是一个展示如何使用 SakuraEngine 的 SystemApp 接口创建窗口和处理事件的最简单示例。

## 功能

- 创建一个可调整大小的窗口
- 处理基本的系统事件（退出、窗口关闭、调整大小等）
- 响应键盘输入（按 ESC 键退出）
- 响应鼠标点击
- 显示窗口信息（大小、DPI 缩放等）

## 构建

使用 SB 构建系统：

```bash
# 在项目根目录执行
dotnet run --project tools/SB/SB.csproj -- build --target SystemWindowExample
```

## 运行

构建完成后，可执行文件将位于：
- Windows: `.build/[toolchain]/bin/SystemWindowExample.exe`
- Linux/macOS: `.build/[toolchain]/bin/SystemWindowExample`

## 代码结构

- `main.cpp` - 主程序文件，包含窗口创建和事件循环
- `build.cs` - SB 构建脚本，定义编译配置和依赖

## 关键 API

- `SystemApp::Create()` - 创建系统应用程序实例
- `SystemApp::create_window()` - 创建窗口
- `SystemEventQueue::pump_messages()` - 处理事件队列
- `ISystemEventSource::poll_event()` - 轮询平台事件

## 事件处理

示例处理了以下事件：
- `SKR_SYSTEM_EVENT_QUIT` - 系统退出事件
- `SKR_SYSTEM_EVENT_WINDOW_CLOSE_REQUESTED` - 窗口关闭请求
- `SKR_SYSTEM_EVENT_WINDOW_RESIZED` - 窗口大小调整
- `SKR_SYSTEM_EVENT_KEY_DOWN` - 键盘按下
- `SKR_SYSTEM_EVENT_MOUSE_BUTTON_DOWN` - 鼠标按下