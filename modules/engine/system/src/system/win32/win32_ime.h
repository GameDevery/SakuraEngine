#pragma once
#include "SkrSystem/IME.h"
#include <Windows.h>
#include <imm.h>
#include <mutex>

#pragma comment(lib, "imm32.lib")

namespace skr {

class Win32Window;

// Win32 IME implementation using Windows IMM API
class Win32IME : public IME
{
public:
    Win32IME() SKR_NOEXCEPT;
    ~Win32IME() SKR_NOEXCEPT override;

    // IME interface implementation
    void start_text_input(SystemWindow* window) override;
    void start_text_input_ex(SystemWindow* window, 
                            EIMEInputType input_type,
                            EIMECapitalization capitalization = EIMECapitalization::Sentences,
                            bool autocorrect = true,
                            bool multiline = false) override;
    void stop_text_input() override;
    bool is_text_input_active() const override;
    SystemWindow* get_text_input_window() const override;
    
    void set_text_input_area(const IMETextInputArea& area) override;
    IMETextInputArea get_text_input_area() const override;
    
    void set_event_callbacks(const IMEEventCallbacks& callbacks) override;
    
    bool is_screen_keyboard_supported() const override;
    
    IMECompositionInfo get_composition_info() const override;
    IMECandidateInfo get_candidates_info() const override;
    
    void clear_composition() override;
    void commit_composition() override;

    // Win32 specific - process window messages
    bool process_message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result);

private:
    // Internal state management
    struct CompositionState
    {
        skr::String text;
        int32_t cursor_pos = 0;
        int32_t selection_start = -1;
        int32_t selection_length = -1;
        bool active = false;
    };

    struct CandidateState
    {
        skr::Vector<skr::String> candidates;
        int32_t selected_index = -1;
        int32_t page_start = 0;
        int32_t page_size = 9;  // Standard 1-9 selection
        int32_t total_candidates = 0;
        bool visible = false;
        EIMECandidateLayout layout = EIMECandidateLayout::Vertical;
    };
    
    // Helper methods
    void update_ime_position(HWND hwnd);
    void handle_composition_string(HWND hwnd, LPARAM lParam);
    void handle_composition_end(HWND hwnd);
    void handle_candidate_list(HWND hwnd);
    void retrieve_composition_string(HIMC hIMC, DWORD type, skr::String& out_str);
    void retrieve_candidate_list(HIMC hIMC);
    void notify_composition_update();
    void notify_candidates_update();
    
    HWND get_hwnd(SystemWindow* window) const;
    void enable_ime(HWND hwnd, bool enable);
    
private:
    Win32Window* active_window_ = nullptr;
    IMEEventCallbacks callbacks_;
    
    // Current state
    mutable std::recursive_mutex state_mutex_;
    CompositionState composition_state_;
    CandidateState candidate_state_;
    IMETextInputArea input_area_ = {};
    
    // Track if we should handle IME messages
    bool ime_enabled_ = false;
    
    // Input type and properties
    EIMEInputType current_input_type_ = EIMEInputType::Text;
    EIMECapitalization current_capitalization_ = EIMECapitalization::Sentences;
};

} // namespace skr