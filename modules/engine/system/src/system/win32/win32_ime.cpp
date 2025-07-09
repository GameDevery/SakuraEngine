#include "win32_ime.h"
#include "win32_system_app.h"
#include "win32_window.h"
#include "SkrCore/log.h"
#include "SkrCore/memory/memory.h"

namespace skr {

Win32IME::Win32IME(Win32SystemApp* app) SKR_NOEXCEPT
    : app_(app)
{
}

Win32IME::~Win32IME() SKR_NOEXCEPT
{
    stop_text_input();
}

void Win32IME::start_text_input(SystemWindow* window)
{
    start_text_input_ex(window, EIMEInputType::Text);
}

void Win32IME::start_text_input_ex(SystemWindow* window, 
                                  EIMEInputType input_type,
                                  EIMECapitalization capitalization,
                                  bool autocorrect,
                                  bool multiline)
{
    if (!window)
    {
        SKR_LOG_ERROR(u8"Cannot start text input with null window");
        return;
    }
    
    std::lock_guard lock(state_mutex_);
    
    // Stop previous session
    if (active_window_)
    {
        stop_text_input();
    }
    
    active_window_ = static_cast<Win32Window*>(window);
    HWND hwnd = active_window_->get_hwnd();
    
    // Store input properties
    current_input_type_ = input_type;
    current_capitalization_ = capitalization;
    
    // Enable IME
    enable_ime(hwnd, true);
    ime_enabled_ = true;
    
    // Update IME position
    update_ime_position(hwnd);
    
    // Notify callbacks
    if (callbacks_.on_composition_start)
    {
        callbacks_.on_composition_start();
    }
}

void Win32IME::stop_text_input()
{
    std::lock_guard lock(state_mutex_);
    
    if (!active_window_)
        return;
        
    HWND hwnd = active_window_->get_hwnd();
    
    // Clear any pending composition
    if (composition_state_.active)
    {
        clear_composition();
    }
    
    // Disable IME
    enable_ime(hwnd, false);
    ime_enabled_ = false;
    
    // Clear state
    active_window_ = nullptr;
    composition_state_ = {};
    candidate_state_ = {};
}

bool Win32IME::is_text_input_active() const
{
    std::lock_guard lock(state_mutex_);
    return active_window_ != nullptr && ime_enabled_;
}

SystemWindow* Win32IME::get_text_input_window() const
{
    std::lock_guard lock(state_mutex_);
    return active_window_;
}

void Win32IME::set_text_input_area(const IMETextInputArea& area)
{
    std::lock_guard lock(state_mutex_);
    
    input_area_ = area;
    
    if (active_window_)
    {
        update_ime_position(active_window_->get_hwnd());
    }
    
    if (callbacks_.on_input_area_change)
    {
        callbacks_.on_input_area_change(area);
    }
}

IMETextInputArea Win32IME::get_text_input_area() const
{
    std::lock_guard lock(state_mutex_);
    return input_area_;
}

void Win32IME::set_event_callbacks(const IMEEventCallbacks& callbacks)
{
    std::lock_guard lock(state_mutex_);
    callbacks_ = callbacks;
}

bool Win32IME::is_screen_keyboard_supported() const
{
    // Windows supports on-screen keyboard for touch devices
    return GetSystemMetrics(SM_DIGITIZER) & NID_READY;
}

IMECompositionInfo Win32IME::get_composition_info() const
{
    std::lock_guard lock(state_mutex_);
    
    IMECompositionInfo info;
    info.text = composition_state_.text;
    info.cursor_pos = composition_state_.cursor_pos;
    info.selection_start = composition_state_.selection_start;
    info.selection_length = composition_state_.selection_length;
    
    return info;
}

IMECandidateInfo Win32IME::get_candidates_info() const
{
    std::lock_guard lock(state_mutex_);
    
    IMECandidateInfo info;
    info.candidates = candidate_state_.candidates;
    info.selected_index = candidate_state_.selected_index;
    info.page_start = candidate_state_.page_start;
    info.page_size = candidate_state_.page_size;
    info.total_candidates = candidate_state_.total_candidates;
    info.preferred_layout = candidate_state_.layout;
    
    return info;
}

void Win32IME::clear_composition()
{
    if (!active_window_)
        return;
        
    HWND hwnd = active_window_->get_hwnd();
    HIMC hIMC = ImmGetContext(hwnd);
    if (hIMC)
    {
        ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
        ImmReleaseContext(hwnd, hIMC);
    }
    
    // Clear state
    composition_state_ = {};
    candidate_state_.visible = false;
    
    // Notify callbacks
    if (callbacks_.on_composition_end)
    {
        callbacks_.on_composition_end(false);
    }
}

void Win32IME::commit_composition()
{
    if (!active_window_ || !composition_state_.active)
        return;
        
    HWND hwnd = active_window_->get_hwnd();
    HIMC hIMC = ImmGetContext(hwnd);
    if (hIMC)
    {
        ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
        ImmReleaseContext(hwnd, hIMC);
    }
}

bool Win32IME::process_message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    // Always handle IME messages if we have an active window
    if (active_window_ && active_window_->get_hwnd() == hwnd)
    {
        std::lock_guard lock(state_mutex_);
        
        switch (msg)
        {
            case WM_IME_SETCONTEXT:
                // Show/hide composition and candidate windows
                if (wParam)
                {
                    update_ime_position(hwnd);
                }
                return false;  // Let DefWindowProc handle it
                
            case WM_IME_STARTCOMPOSITION:
                composition_state_.active = true;
                if (callbacks_.on_composition_start)
                {
                    callbacks_.on_composition_start();
                }
                return false;
                
            case WM_IME_COMPOSITION:
                handle_composition_string(hwnd, lParam);
                // Chromium style: prevent DefWindowProc from generating WM_CHAR
                // Only do this for result string to avoid interfering with composition
                if (lParam & GCS_RESULTSTR)
                {
                    result = 0;
                    return true;  // Handled, don't call DefWindowProc
                }
                return false;  // Let DefWindowProc handle composition display
                
            case WM_IME_ENDCOMPOSITION:
                handle_composition_end(hwnd);
                return false;
                
            case WM_IME_NOTIFY:
                if (wParam == IMN_OPENCANDIDATE || wParam == IMN_CHANGECANDIDATE)
                {
                    handle_candidate_list(hwnd);
                }
                else if (wParam == IMN_CLOSECANDIDATE)
                {
                    candidate_state_.visible = false;
                    candidate_state_.candidates.clear();
                    notify_candidates_update();
                }
                return false;
            
            case WM_CHAR:
            case WM_SYSCHAR:
                // Chromium style: Always process WM_CHAR without special IME checks
                // The key is that we prevented WM_IME_CHAR from being generated
                // by handling GCS_RESULTSTR and returning true
                
                if (wParam >= 32)  // Filter out control characters
                {
                    // Convert UTF-16 to UTF-8
                    wchar_t utf16[2] = { static_cast<wchar_t>(wParam), 0 };
                    char utf8[5] = {};
                    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, utf16, 1, utf8, sizeof(utf8), nullptr, nullptr);
                    
                    if (callbacks_.on_text_input && utf8_len > 0)
                    {
                        // Create skr::String from UTF-8 data
                        skr::String text((const char8_t*)utf8, utf8_len);
                        SKR_LOG_DEBUG(u8"WM_CHAR: Processing character 0x%X", wParam);
                        callbacks_.on_text_input(text);
                    }
                }
                result = 0;
                return true;  // Always handle WM_CHAR
        }
    }
    
    // For windows that aren't active IME windows, still handle WM_CHAR for direct input
    if (msg == WM_CHAR || msg == WM_SYSCHAR)
    {
        // Only process if IME is not globally enabled
        if (!ime_enabled_ && wParam >= 32)
        {
            // Convert UTF-16 to UTF-8
            wchar_t utf16[2] = { static_cast<wchar_t>(wParam), 0 };
            char utf8[5] = {};
            int utf8_len = WideCharToMultiByte(CP_UTF8, 0, utf16, 1, utf8, sizeof(utf8), nullptr, nullptr);
            
            if (callbacks_.on_text_input && utf8_len > 0)
            {
                // Create skr::String from UTF-8 data
                skr::String text((const char8_t*)utf8, utf8_len);
                callbacks_.on_text_input(text);
            }
            result = 0;
            return true;
        }
    }
    
    return false;
}

void Win32IME::update_ime_position(HWND hwnd)
{
    HIMC hIMC = ImmGetContext(hwnd);
    if (!hIMC)
        return;
        
    // Set composition window position
    COMPOSITIONFORM cf = {};
    cf.dwStyle = CFS_POINT;
    cf.ptCurrentPos.x = input_area_.position.x;
    cf.ptCurrentPos.y = input_area_.position.y + input_area_.cursor_height;
    ImmSetCompositionWindow(hIMC, &cf);
    
    // Set candidate window position
    CANDIDATEFORM cdf = {};
    cdf.dwIndex = 0;
    cdf.dwStyle = CFS_CANDIDATEPOS;
    cdf.ptCurrentPos = cf.ptCurrentPos;
    ImmSetCandidateWindow(hIMC, &cdf);
    
    ImmReleaseContext(hwnd, hIMC);
}

void Win32IME::handle_composition_string(HWND hwnd, LPARAM lParam)
{
    HIMC hIMC = ImmGetContext(hwnd);
    if (!hIMC)
        return;
 
    bool updated = false;
    
    // Get composition string
    if (lParam & GCS_COMPSTR)
    {
        retrieve_composition_string(hIMC, GCS_COMPSTR, composition_state_.text);
        updated = true;
        SKR_LOG_DEBUG(u8"IME composition string: %s", composition_state_.text.c_str());
    }
    
    // Get cursor position
    if (lParam & GCS_CURSORPOS)
    {
        composition_state_.cursor_pos = ImmGetCompositionStringW(hIMC, GCS_CURSORPOS, nullptr, 0);
    }
    
    // Get attributes for selection
    if (lParam & GCS_COMPATTR)
    {
        DWORD attr_size = ImmGetCompositionStringW(hIMC, GCS_COMPATTR, nullptr, 0);
        if (attr_size > 0)
        {
            skr::Vector<BYTE> attrs;
            attrs.resize_zeroed(attr_size);
            ImmGetCompositionStringW(hIMC, GCS_COMPATTR, attrs.data(), attr_size);
            
            // Find selection range (ATTR_TARGET_CONVERTED or ATTR_TARGET_NOTCONVERTED)
            composition_state_.selection_start = -1;
            composition_state_.selection_length = -1;
            
            for (DWORD i = 0; i < attr_size; ++i)
            {
                if (attrs[i] == ATTR_TARGET_CONVERTED || attrs[i] == ATTR_TARGET_NOTCONVERTED)
                {
                    if (composition_state_.selection_start == -1)
                    {
                        composition_state_.selection_start = i;
                        composition_state_.selection_length = 1;
                    }
                    else
                    {
                        composition_state_.selection_length++;
                    }
                }
            }
        }
    }
    
    // Get result string (committed text)
    if (lParam & GCS_RESULTSTR)
    {
        skr::String result_str;
        retrieve_composition_string(hIMC, GCS_RESULTSTR, result_str);
        
        if (!result_str.is_empty() && callbacks_.on_text_input)
        {
            SKR_LOG_DEBUG(u8"IME result string: %s (will block DefWindowProc)", result_str.c_str());
            callbacks_.on_text_input(result_str);
        }
    }
    
    ImmReleaseContext(hwnd, hIMC);
    
    if (updated)
    {
        notify_composition_update();
    }
}

void Win32IME::handle_composition_end(HWND hwnd)
{
    composition_state_.active = false;
    composition_state_.text.clear();
    composition_state_.cursor_pos = 0;
    composition_state_.selection_start = -1;
    composition_state_.selection_length = -1;
    
    if (callbacks_.on_composition_end)
    {
        callbacks_.on_composition_end(true);
    }
}

void Win32IME::handle_candidate_list(HWND hwnd)
{
    HIMC hIMC = ImmGetContext(hwnd);
    if (!hIMC)
        return;
        
    retrieve_candidate_list(hIMC);
    ImmReleaseContext(hwnd, hIMC);
    
    notify_candidates_update();
}

void Win32IME::retrieve_composition_string(HIMC hIMC, DWORD type, skr::String& out_str)
{
    LONG len = ImmGetCompositionStringW(hIMC, type, nullptr, 0);
    if (len <= 0)
    {
        out_str.clear();
        return;
    }
    
    skr::Vector<wchar_t> buffer;
    buffer.resize_default((len / sizeof(wchar_t)) + 1);
    ImmGetCompositionStringW(hIMC, type, buffer.data(), len);
    buffer[len / sizeof(wchar_t)] = 0;
    
    // Create string without the null terminator
    // Note: len is the byte count, not character count
    out_str = skr::String::FromUtf16((const skr_char16*)buffer.data(), len / sizeof(wchar_t));
}

void Win32IME::retrieve_candidate_list(HIMC hIMC)
{
    candidate_state_.candidates.clear();
    
    DWORD size = ImmGetCandidateListW(hIMC, 0, nullptr, 0);
    if (size == 0)
        return;
        
    skr::Vector<BYTE> buffer;
    buffer.resize_default(size);
    CANDIDATELIST* list = reinterpret_cast<CANDIDATELIST*>(buffer.data());
    
    if (ImmGetCandidateListW(hIMC, 0, list, size) == 0)
        return;
        
    candidate_state_.total_candidates = list->dwCount;
    candidate_state_.selected_index = list->dwSelection;
    candidate_state_.page_start = list->dwPageStart;
    candidate_state_.page_size = list->dwPageSize;
    candidate_state_.visible = true;
    
    // Extract candidates
    for (DWORD i = 0; i < list->dwCount && i < 100; ++i)  // Limit to prevent excessive memory use
    {
        DWORD offset = list->dwOffset[i];
        wchar_t* candidate = reinterpret_cast<wchar_t*>(reinterpret_cast<BYTE*>(list) + offset);
        
        // Convert to UTF-8
        int utf8_len = WideCharToMultiByte(CP_UTF8, 0, candidate, -1, nullptr, 0, nullptr, nullptr);
        skr::Vector<char> utf8_buffer;
        utf8_buffer.resize_default(utf8_len);
        WideCharToMultiByte(CP_UTF8, 0, candidate, -1, utf8_buffer.data(), utf8_len, nullptr, nullptr);
        
        candidate_state_.candidates.emplace((const char8_t*)utf8_buffer.data());
    }
}

void Win32IME::notify_composition_update()
{
    if (callbacks_.on_composition_update)
    {
        IMECompositionInfo info = get_composition_info();
        callbacks_.on_composition_update(info);
    }
}

void Win32IME::notify_candidates_update()
{
    if (callbacks_.on_candidates_update)
    {
        IMECandidateInfo info = get_candidates_info();
        callbacks_.on_candidates_update(info);
    }
}

HWND Win32IME::get_hwnd(SystemWindow* window) const
{
    if (!window)
        return nullptr;
        
    return static_cast<HWND>(window->get_native_handle());
}

void Win32IME::enable_ime(HWND hwnd, bool enable)
{
    ImmAssociateContextEx(hwnd, nullptr, enable ? IACE_DEFAULT : 0);
}

// Factory methods
#ifdef SKR_PLAT_WINDOWS
IME* IME::Create(SystemApp* app)
{
    return SkrNew<Win32IME>(static_cast<Win32SystemApp*>(app));
}

void IME::Destroy(IME* ime)
{
    if (ime)
    {
        SkrDelete(ime);
    }
}
#endif

} // namespace skr