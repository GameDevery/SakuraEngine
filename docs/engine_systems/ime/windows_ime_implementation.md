# Windows IME 实现详解

## 概述

SakuraEngine 的 Windows IME (Input Method Editor) 实现提供了完整的中日韩文字输入支持。系统采用 Chromium 风格的消息处理策略，通过主动控制 Windows 消息流来避免常见的重复输入问题。

## 系统架构

### 核心组件

```
┌─────────────────────────────────────────────┐
│                Win32IME                      │
│  ┌─────────────────┐  ┌─────────────────┐  │
│  │ Message Handler │  │  State Manager  │  │
│  └─────────────────┘  └─────────────────┘  │
│  ┌─────────────────┐  ┌─────────────────┐  │
│  │ Composition     │  │   Callbacks     │  │
│  │    Manager      │  │    System       │  │
│  └─────────────────┘  └─────────────────┘  │
└─────────────────────────────────────────────┘
                    ↓
         Windows IMM API (imm32.dll)
```

### 类设计

```cpp
class Win32IME : public IME
{
    // 状态管理
    CompositionState composition_state_;  // 组合字符串状态
    CandidateState candidate_state_;      // 候选词状态
    bool ime_enabled_ = false;            // IME 激活状态
    
    // 窗口管理
    Win32Window* active_window_ = nullptr;
    IMETextInputArea input_area_;
    
    // 回调系统
    IMEEventCallbacks callbacks_;
};
```

## 消息处理流程

### Chromium 风格的核心策略

1. **阻止 DefWindowProc 生成 WM_IME_CHAR**
2. **统一通过 WM_CHAR 处理所有文本输入**
3. **避免重复输入问题**

### 消息处理实现

```cpp
bool Win32IME::process_message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    switch (msg)
    {
        case WM_IME_COMPOSITION:
            handle_composition_string(hwnd, lParam);
            
            // 关键：收到结果时阻止 DefWindowProc
            if (lParam & GCS_RESULTSTR)
            {
                result = 0;
                return true;  // 阻止生成 WM_IME_CHAR
            }
            return false;  // 其他情况继续默认处理
            
        case WM_CHAR:
        case WM_SYSCHAR:
            // 统一处理所有字符输入
            if (wParam >= 32)  // 过滤控制字符
            {
                ProcessChar(wParam);
            }
            result = 0;
            return true;
    }
}
```

## 详细实现

### 1. IME 会话管理

#### 启动文本输入

```cpp
void Win32IME::start_text_input(SystemWindow* window)
{
    // 1. 激活 IME
    active_window_ = static_cast<Win32Window*>(window);
    HWND hwnd = active_window_->get_hwnd();
    
    // 2. 启用 IME 上下文
    ImmAssociateContextEx(hwnd, nullptr, IACE_DEFAULT);
    ime_enabled_ = true;
    
    // 3. 设置输入位置
    update_ime_position(hwnd);
    
    // 4. 通知回调
    if (callbacks_.on_composition_start)
        callbacks_.on_composition_start();
}
```

#### 停止文本输入

```cpp
void Win32IME::stop_text_input()
{
    if (!active_window_) return;
    
    // 1. 清理未完成的组合
    if (composition_state_.active)
    {
        clear_composition();
    }
    
    // 2. 禁用 IME
    ImmAssociateContextEx(hwnd, nullptr, 0);
    ime_enabled_ = false;
    
    // 3. 重置状态
    active_window_ = nullptr;
    composition_state_ = {};
    candidate_state_ = {};
}
```

### 2. 组合字符串处理

#### 获取组合字符串

```cpp
void Win32IME::handle_composition_string(HWND hwnd, LPARAM lParam)
{
    HIMC hIMC = ImmGetContext(hwnd);
    if (!hIMC) return;
    
    // 1. 获取正在输入的文本（如拼音）
    if (lParam & GCS_COMPSTR)
    {
        retrieve_composition_string(hIMC, GCS_COMPSTR, composition_state_.text);
        SKR_LOG_DEBUG(u8"IME composition: %s", composition_state_.text.c_str());
    }
    
    // 2. 获取最终结果（如选中的汉字）
    if (lParam & GCS_RESULTSTR)
    {
        skr::String result_str;
        retrieve_composition_string(hIMC, GCS_RESULTSTR, result_str);
        
        if (!result_str.is_empty() && callbacks_.on_text_input)
        {
            SKR_LOG_DEBUG(u8"IME result: %s", result_str.c_str());
            callbacks_.on_text_input(result_str);
        }
    }
    
    ImmReleaseContext(hwnd, hIMC);
}
```

#### 字符串转换

```cpp
void Win32IME::retrieve_composition_string(HIMC hIMC, DWORD type, skr::String& out_str)
{
    // 1. 获取字符串长度（字节数）
    LONG len = ImmGetCompositionStringW(hIMC, type, nullptr, 0);
    if (len <= 0)
    {
        out_str.clear();
        return;
    }
    
    // 2. 分配缓冲区并获取内容
    skr::Vector<wchar_t> buffer;
    buffer.resize_default((len / sizeof(wchar_t)) + 1);
    ImmGetCompositionStringW(hIMC, type, buffer.data(), len);
    
    // 3. 转换为 UTF-8
    out_str = skr::String::FromUtf16((const skr_char16*)buffer.data(), 
                                     len / sizeof(wchar_t));
}
```

### 3. IME 位置设置

```cpp
void Win32IME::update_ime_position(HWND hwnd)
{
    HIMC hIMC = ImmGetContext(hwnd);
    if (!hIMC) return;
    
    // 设置组合窗口位置
    COMPOSITIONFORM cf = {};
    cf.dwStyle = CFS_POINT;
    cf.ptCurrentPos.x = input_area_.position.x;
    cf.ptCurrentPos.y = input_area_.position.y + input_area_.cursor_height;
    ImmSetCompositionWindow(hIMC, &cf);
    
    // 设置候选词窗口位置
    CANDIDATEFORM cdf = {};
    cdf.dwIndex = 0;
    cdf.dwStyle = CFS_CANDIDATEPOS;
    cdf.ptCurrentPos = cf.ptCurrentPos;
    ImmSetCandidateWindow(hIMC, &cdf);
    
    ImmReleaseContext(hwnd, hIMC);
}
```

### 4. 候选词列表处理

#### 现代输入法的限制

微软拼音等现代输入法使用 TSF (Text Services Framework) 而非传统 IMM API，因此：

1. **候选词窗口由输入法自行管理**
2. **通过 IMM API 无法获取候选词列表**
3. **这是正常行为，不是 bug**

```cpp
void Win32IME::retrieve_candidate_list(HIMC hIMC)
{
    // 尝试获取候选词列表
    DWORD size = ImmGetCandidateListW(hIMC, 0, nullptr, 0);
    if (size == 0)
    {
        // 微软拼音等现代输入法会返回 0
        // 这是正常的，不需要特殊处理
        return;
    }
    
    // 传统输入法的处理...
}
```

## 常见问题和解决方案

### 1. 文字重复输入

**问题**：输入中文时，每个字都出现两次。

**原因**：Windows 会发送两个消息：
- `WM_IME_COMPOSITION` 带 `GCS_RESULTSTR`
- `WM_CHAR` 或 `WM_IME_CHAR`

**解决方案**：Chromium 风格 - 处理 `GCS_RESULTSTR` 时返回 `true` 阻止 DefWindowProc。

### 2. 微软拼音数字选择候选词

**问题**：用数字键选择候选词时，文字不显示。

**原因**：微软拼音在数字选择时可能直接发送 `WM_CHAR`，跳过 `WM_IME_COMPOSITION`。

**解决方案**：统一处理所有 `WM_CHAR` 消息，不做 IME 状态检查。

### 3. 候选词列表获取失败

**问题**：`ImmGetCandidateListW` 返回 0。

**原因**：现代输入法使用 TSF，不通过 IMM API 暴露候选词。

**解决方案**：接受这个限制，让输入法自行管理候选词窗口。

## 与其他系统的集成

### ImGui 集成示例

```cpp
// 在 ImGuiBackend 中设置 IME 回调
void ImGuiBackendSystem::setup_ime_callbacks()
{
    IMEEventCallbacks callbacks;
    
    callbacks.on_text_input = [this](const skr::String& text) {
        if (_context && !text.is_empty())
        {
            ImGuiContext* prev_ctx = ImGui::GetCurrentContext();
            ImGui::SetCurrentContext(_context);
            _context->IO.AddInputCharactersUTF8(
                reinterpret_cast<const char*>(text.c_str()));
            ImGui::SetCurrentContext(prev_ctx);
        }
    };
    
    callbacks.on_composition_update = [this](const IMECompositionInfo& info) {
        // 可以显示正在输入的拼音
        SKR_LOG_DEBUG(u8"Composing: %s", info.text.c_str());
    };
    
    _ime->set_event_callbacks(callbacks);
}
```

## 调试技巧

### 1. 启用详细日志

```cpp
// 在关键位置添加日志
SKR_LOG_DEBUG(u8"IME composition: %s", composition_text.c_str());
SKR_LOG_DEBUG(u8"IME result: %s (blocking DefWindowProc)", result_text.c_str());
SKR_LOG_DEBUG(u8"WM_CHAR: 0x%X", wParam);
```

### 2. 消息流追踪

使用 Spy++ 或类似工具观察消息序列：
- 正常输入：`WM_IME_COMPOSITION` → `WM_CHAR`
- 数字选择：可能只有 `WM_CHAR`

### 3. 编码验证

```cpp
// 打印十六进制确认 UTF-8 编码
for (size_t i = 0; i < text.size(); ++i)
{
    SKR_LOG_DEBUG(u8"Byte %d: 0x%02X", i, (unsigned char)text.c_str()[i]);
}
```

## 性能考虑

1. **避免频繁的上下文获取**
   ```cpp
   HIMC hIMC = ImmGetContext(hwnd);
   // 批量处理
   ImmReleaseContext(hwnd, hIMC);
   ```

2. **字符串转换优化**
   - 预分配缓冲区
   - 避免不必要的复制

3. **状态缓存**
   - 缓存 IME 位置信息
   - 减少重复计算

## 未来改进方向

### 1. TSF 支持

实现 Text Services Framework 以获得：
- 完整的候选词列表访问
- 更好的输入法集成
- 支持手写和语音输入

### 2. 组合字符串显示

在应用程序中显示正在输入的拼音：
- 提供更好的用户反馈
- 支持内嵌编辑

### 3. 多显示器优化

改进 IME 窗口定位：
- 跨显示器边界处理
- DPI 感知定位

## 参考资料

- [Windows IME API 文档](https://docs.microsoft.com/en-us/windows/win32/intl/input-method-editor)
- [Chromium IME 实现](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/ui/base/ime/)
- [Text Services Framework](https://docs.microsoft.com/en-us/windows/win32/tsf/text-services-framework)

## 总结

SakuraEngine 的 Windows IME 实现采用了经过验证的 Chromium 风格消息处理策略，成功解决了常见的重复输入和兼容性问题。虽然对现代输入法的候选词访问有限制，但整体方案简洁可靠，完全满足游戏引擎的文本输入需求。