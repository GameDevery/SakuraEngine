#include <SkrImGui/imgui_backend.hpp>
#include <SkrImGui/imgui_render_backend.hpp>
#include <SkrImGui/imgui_system_event_handler.hpp>
#include <SkrSystem/system.h>
#include <SkrSystem/window.h>
#include <SkrSystem/window_manager.h>
#include <SkrSystem/IME.h>
#include <SkrCore/log.hpp>
#include <SkrCore/memory/memory.h>
#include <chrono>
#include <cstring>

namespace skr
{

// Clipboard functions
static const char* ImGui_ImplSkrSystem_GetClipboardText(ImGuiContext* ctx)
{
    ImGuiIO& io = ctx->IO;
    ImGuiBackend* backend = static_cast<ImGuiBackend*>(io.BackendPlatformUserData);
    if (backend && backend->system_app())
    {
        auto ime = backend->system_app()->get_ime();
        backend->_clipboard = ime->get_clipboard_text();
        return backend->_clipboard.c_str_raw();
    }
    return nullptr;
}

static void ImGui_ImplSkrSystem_SetClipboardText(ImGuiContext* ctx, const char* text)
{
    ImGuiIO& io = ctx->IO;
    ImGuiBackend* backend = static_cast<ImGuiBackend*>(io.BackendPlatformUserData);
    if (backend && backend->system_app())
    {
        auto ime = backend->system_app()->get_ime();
        ime->set_clipboard_text(skr::String((const char8_t*)text));
    }
}

// ctor & dtor
ImGuiBackend::ImGuiBackend()
{
}

ImGuiBackend::~ImGuiBackend()
{
    if (is_created())
    {
        destroy();
    }
}

// imgui context
void ImGuiBackend::apply_context()
{
    SKR_ASSERT(is_created() && "please call create() before apply");
    SKR_ASSERT(!_renderer_backend.is_empty() && "please set render backend before apply");

    ImGui::SetCurrentContext(_context);
}

// Create overload with SystemApp integration
void ImGuiBackend::create(
    SystemApp*                     system_app,
    const ImGuiWindowCreateInfo&   main_wnd_create_info,
    RCUnique<ImGuiRendererBackend> backend)
{
    SKR_ASSERT(!is_created() && "multi create context");
    // SystemApp can be null in legacy mode

    _system_app = system_app;
    _renderer_backend = std::move(backend);

    // Create context
    _context = ImGui::CreateContext();

    // Create main window through WindowManager (if not in legacy mode)
    if (_system_app && !_system_window)
    {
        auto* window_manager = _system_app->get_window_manager();
        
        SystemWindowCreateInfo sys_create_info;
        sys_create_info.title = main_wnd_create_info.title;
        sys_create_info.size = main_wnd_create_info.size;
        sys_create_info.pos = main_wnd_create_info.pos; // Both use Optional<uint2>
        sys_create_info.is_topmost = main_wnd_create_info.is_topmost;
        sys_create_info.is_tooltip = main_wnd_create_info.is_tooltip;
        sys_create_info.is_borderless = main_wnd_create_info.is_borderless;
        sys_create_info.is_resizable = main_wnd_create_info.is_resizable;
        _system_window = window_manager->create_window(sys_create_info);
    }
    
    // Create event handler and register with event queue (if we have SystemApp)
    if (_system_app)
    {
        _event_handler = SkrNew<ImGuiSystemEventHandler>(this);
        auto* event_queue = _system_app->get_event_queue();
        event_queue->add_handler(_event_handler);
    }

    // Setup ImGui platform backend
    {
        ImGuiContext* cache = ImGui::GetCurrentContext();
        ImGui::SetCurrentContext(_context);
        
        // Initialize ImGui IO for SkrSystem
        ImGuiIO& io = _context->IO;
        io.BackendPlatformUserData = this;  // Store backend pointer for callbacks
        io.BackendPlatformName = "imgui_impl_skr_system";
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;           // We can honor GetMouseCursor() values (optional)
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;            // We can honor io.WantSetMousePos requests (optional, rarely used)
        
        // Enable multi-viewport support if we have event system support
        if (_system_app)
        {
            io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;  // We can create multi-viewports on the Platform side (optional)
        }
        
        // Setup platform functions
        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
        platform_io.Platform_SetClipboardTextFn = ImGui_ImplSkrSystem_SetClipboardText;
        platform_io.Platform_GetClipboardTextFn = ImGui_ImplSkrSystem_GetClipboardText;
        platform_io.Platform_SetImeDataFn = [](ImGuiContext* ctx, ImGuiViewport* viewport, ImGuiPlatformImeData* data) {
            ImGuiIO& io = ctx->IO;
            ImGuiBackend* backend = static_cast<ImGuiBackend*>(io.BackendPlatformUserData);
            if (backend && backend->_ime && backend->_system_window)
            {
                if (data->WantVisible)
                {
                    // Set input area for IME window positioning
                    IMETextInputArea area;
                    area.position = {
                        static_cast<int32_t>(data->InputPos.x - viewport->Pos.x),
                        static_cast<int32_t>(data->InputPos.y - viewport->Pos.y)
                    };
                    area.size = {
                        200,  // Default width
                        static_cast<uint32_t>(data->InputLineHeight)
                    };
                    area.cursor_height = static_cast<uint32_t>(data->InputLineHeight);
                    
                    backend->_ime->set_text_input_area(area);
                }
                
                // Note: We don't start/stop IME here because UpdateIMEState() already handles it
                // based on WantTextInput. This callback is mainly for positioning the IME window.
            }
        };

        ImGui::SetCurrentContext(cache);
    }
    
    // Setup IME if available
    if (_system_app)
    {
        _ime = _system_app->get_ime();
        if (_ime)
        {
            SetupIMECallbacks();
        }
    }

    // Initialize render backend
    {
        _context->IO.BackendRendererUserData = _renderer_backend.get();
        _context->IO.BackendRendererName     = "Sakura ImGui Renderer";
        _context->IO.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
        _renderer_backend->setup_io(_context->IO);
    }

    // Setup main viewport platform handles
    // Our mouse update function expect PlatformHandle to be filled for the main viewport
    auto* main_viewport = _context->Viewports[0];
    if (_system_window)
    {
        // Store window ID as PlatformHandle (similar to SDL3's approach)
        // This allows us to find viewport by window later
        main_viewport->PlatformHandle = _system_window;
        main_viewport->PlatformHandleRaw = _system_window->get_native_view();
    }
    
    // Initialize main window render data
    _renderer_backend->create_main_window(main_viewport);
    
    // Show the window
    if (_system_window)
    {
        _system_window->show();
    }
    
    // Initialize time tracking
    last_frame_time_ = std::chrono::steady_clock::now();
}

// New process_events method using SystemEventQueue
void ImGuiBackend::process_events()
{
    SKR_ASSERT(is_created() && "please create context before processing events");
    
    if (_system_app)
    {
        // Events are automatically dispatched to our handler through the event queue
        // We just need to pump the event queue
        auto* event_queue = _system_app->get_event_queue();
        event_queue->pump_messages();
    }
    
    // Update ImGui display size (every frame to accommodate for window resizing)
    if (_system_window)
    {
        auto window_size = _system_window->get_size();
        
        // Handle minimized window
        int w = window_size.x;
        int h = window_size.y;
        if (_system_window->is_minimized())
            w = h = 0;
        
        _context->IO.DisplaySize = ImVec2((float)w, (float)h);
        
        // Update framebuffer scale
        if (w > 0 && h > 0)
        {
            auto pixel_ratio = _system_window->get_pixel_ratio();
            _context->IO.DisplayFramebufferScale = ImVec2(pixel_ratio, pixel_ratio);
        }
    }
}

void ImGuiBackend::begin_frame()
{
    SKR_ASSERT(is_created() && "please create context before begin frame");
    SKR_ASSERT(ImGui::GetCurrentContext() == _context && "context mismatch");

    apply_context();
    
    // Update delta time (use member variable instead of static)
    auto current_time = std::chrono::steady_clock::now();
    float delta_time = std::chrono::duration<float>(current_time - last_frame_time_).count();
    _context->IO.DeltaTime = delta_time > 0.0f ? delta_time : (1.0f / 60.0f);
    last_frame_time_ = current_time;
    
    // Update IME state based on ImGui needs
    UpdateIMEState();
    
    // Update mouse cursor
    UpdateMouseCursor();
    
    ImGui::NewFrame();
    _renderer_backend->begin_frame();
}

// Destroy with SystemApp cleanup
void ImGuiBackend::destroy()
{
    SKR_ASSERT(is_created() && "try destroy context before create");

    // Wait rendering done
    _renderer_backend->wait_rendering_done();
    
    // Stop IME if active
    if (_ime && _ime->is_text_input_active())
    {
        _ime->stop_text_input();
    }

    // Unregister event handler
    if (_event_handler && _system_app)
    {
        auto* event_queue = _system_app->get_event_queue();
        event_queue->remove_handler(_event_handler);
        SkrDelete(_event_handler);
        _event_handler = nullptr;
    }

    // Destroy all textures
    {
        for (ImTextureData* tex : _context->PlatformIO.Textures)
        {
            if (tex->RefCount == 1)
            {
                _renderer_backend->destroy_texture(tex);
                tex->TexID  = 0;
                tex->Status = ImTextureStatus_Destroyed;
            }
        }
    }

    // Clean up render data
    if (_context->Viewports.Size > 0)
    {
        _renderer_backend->destroy_main_window(_context->Viewports[0]);
    }

    // Reset platform backend callbacks and user data
    {
        _context->IO.BackendPlatformUserData = nullptr;
        _context->IO.BackendPlatformName = nullptr;
    }
    
    // Reset render backend callbacks and user data
    {
        _context->IO.BackendRendererUserData = nullptr;
        _context->IO.BackendRendererName     = nullptr;
    }

    // Destroy render backend
    _renderer_backend.reset();

    // Destroy main window through WindowManager
    if (_system_window && _system_app)
    {
        auto* window_manager = _system_app->get_window_manager();
        window_manager->destroy_window(_system_window);
        _system_window = nullptr;
    }

    // Destroy context
    ImGui::DestroyContext(_context);
    _context = nullptr;
    
    // Clean up owned SystemApp (legacy mode)
    if (_owns_system_app && _system_app)
    {
        SystemApp::Destroy(_system_app);
        _owns_system_app = false;
    }
    _system_app = nullptr;
    
    // Clear IME references
    _ime = nullptr;
    _ime_callbacks_set = false;
    _ime_active_state = false;
}

void ImGuiBackend::end_frame()
{
    SKR_ASSERT(is_created() && "please create context before end frame");
    SKR_ASSERT(ImGui::GetCurrentContext() == _context && "context mismatch");

    ImGui::EndFrame();
    if (_context->IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
    }
    _renderer_backend->end_frame();
}

void ImGuiBackend::collect()
{
    SKR_ASSERT(is_created() && "please create context before render");
    SKR_ASSERT(ImGui::GetCurrentContext() == _context && "context mismatch");

    // Update textures
    for (ImTextureData* tex : ImGui::GetPlatformIO().Textures)
    {
        switch (tex->Status)
        {
        case ImTextureStatus_WantCreate:
            _renderer_backend->create_texture(tex);
            SKR_ASSERT(tex->Status == ImTextureStatus_OK);
            break;
        case ImTextureStatus_WantUpdates:
            _renderer_backend->update_texture(tex);
            SKR_ASSERT(tex->Status == ImTextureStatus_OK);
            break;
        case ImTextureStatus_WantDestroy:
            if (tex->UnusedFrames >= _renderer_backend->backbuffer_count())
            {
                _renderer_backend->destroy_texture(tex);
                SKR_ASSERT(tex->Status == ImTextureStatus_Destroyed);
            }
            break;
        default:
            break;
        }
    }
    
    // Handle resize if triggered by event handler
    if (_event_handler && _event_handler->want_resize().comsume())
    {
        auto size = _system_window->get_size();
        _renderer_backend->resize_main_window(
            _context->Viewports[0],
            { (float)size.x, (float)size.y }
        );
    }

    // Render main window
    ImGui::Render();
}

void ImGuiBackend::render()
{
    _renderer_backend->render_main_window(_context->Viewports[0]);

    // Render other viewports
    if (_context->IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::RenderPlatformWindowsDefault();
    }
}

void ImGuiBackend::acquire_next_frame()
{
    SKR_ASSERT(is_created() && "please create context before acquire next frame");
    // Pass the main viewport to acquire_next_frame
    _renderer_backend->acquire_next_frame(_context->Viewports[0]);
}

// Legacy compatibility - create without SystemApp
void ImGuiBackend::create(const ImGuiWindowCreateInfo& main_wnd_create_info, RCUnique<ImGuiRendererBackend> backend)
{
    // For backward compatibility, create a default SystemApp
    auto* default_app = SystemApp::Create();
    if (!default_app)
    {
        SKR_LOG_ERROR(u8"Failed to create default SystemApp for legacy ImGui mode");
        return;
    }
    
    _owns_system_app = true;
    
    // Call the new create method
    create(default_app, main_wnd_create_info, std::move(backend));
}

// Legacy compatibility - pump_message delegates to process_events
void ImGuiBackend::pump_message()
{
    if (_system_app)
    {
        process_events();
    }
    else
    {
        SKR_LOG_WARN(u8"pump_message called without SystemApp - no events processed");
    }
}

// Helper to update mouse cursor
void ImGuiBackend::UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        // TODO: Implement cursor hiding when SystemWindow supports it
    }
    else
    {
        // Show OS mouse cursor and set shape
        // TODO: Implement cursor shape setting when SystemWindow supports it
        // Future implementation would map ImGuiMouseCursor to system cursors:
        // ImGuiMouseCursor_Arrow -> default
        // ImGuiMouseCursor_TextInput -> text/ibeam
        // ImGuiMouseCursor_ResizeAll -> move/all directions
        // ImGuiMouseCursor_ResizeNS -> resize vertical
        // ImGuiMouseCursor_ResizeEW -> resize horizontal
        // ImGuiMouseCursor_ResizeNESW -> resize diagonal 1
        // ImGuiMouseCursor_ResizeNWSE -> resize diagonal 2
        // ImGuiMouseCursor_Hand -> hand/pointer
        // ImGuiMouseCursor_Wait -> wait/hourglass
        // ImGuiMouseCursor_Progress -> progress/app starting
        // ImGuiMouseCursor_NotAllowed -> not allowed/no
    }
}

// imgui functional
void ImGuiBackend::enable_nav(bool enable)
{
    SKR_ASSERT(is_created() && "please create context before set feature");
    if (enable)
    {
        _context->IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        _context->IO.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    }
    else
    {
        _context->IO.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
        _context->IO.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
    }
}

void ImGuiBackend::enable_docking(bool enable)
{
    SKR_ASSERT(is_created() && "please create context before set feature");
    if (enable)
    {
        _context->IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }
    else
    {
        _context->IO.ConfigFlags &= ~ImGuiConfigFlags_DockingEnable;
    }
}

void ImGuiBackend::enable_multi_viewport(bool enable)
{
    SKR_ASSERT(is_created() && "please create context before set feature");
    if (enable)
    {
        _context->IO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }
    else
    {
        _context->IO.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
    }
}

void ImGuiBackend::enable_ini_file(bool enable)
{
    SKR_ASSERT(is_created() && "please create context before set feature");
    if (enable)
    {
        _context->IO.IniFilename = "imgui.ini";
    }
    else
    {
        _context->IO.IniFilename = nullptr;
    }
}

void ImGuiBackend::enable_log_file(bool enable)
{
    SKR_ASSERT(is_created() && "please create context before set feature");
    if (enable)
    {
        _context->IO.LogFilename = "imgui.log";
    }
    else
    {
        _context->IO.LogFilename = nullptr;
    }
}

void ImGuiBackend::enable_transparent_docking(bool enable)
{
    SKR_ASSERT(is_created() && "please create context before set feature");
    _context->IO.ConfigDockingTransparentPayload = enable;
}

void ImGuiBackend::enable_always_tab_bar(bool enable)
{
    SKR_ASSERT(is_created() && "please create context before set feature");
    _context->IO.ConfigDockingAlwaysTabBar = enable;
}

void ImGuiBackend::enable_move_window_by_blank_area(bool enable)
{
    SKR_ASSERT(is_created() && "please create context before set feature");
    _context->IO.ConfigWindowsMoveFromTitleBarOnly = !enable;
}

void ImGuiBackend::enable_high_dpi(bool enable)
{
    SKR_ASSERT(is_created() && "please create context before set feature");
    if (enable)
    {
        _context->IO.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
        _context->IO.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
    }
    else
    {
        _context->IO.ConfigFlags &= ~ImGuiConfigFlags_DpiEnableScaleFonts;
        _context->IO.ConfigFlags &= ~ImGuiConfigFlags_DpiEnableScaleViewports;
    }
}

void ImGuiBackend::SetupIMECallbacks()
{
    if (!_ime || _ime_callbacks_set)
        return;
    
    SKR_LOG_DEBUG(u8"Setting up IME callbacks");
        
    IMEEventCallbacks callbacks;
    
    // Most important callback: receive committed text
    callbacks.on_text_input = [this](const skr::String& text) {
        SKR_LOG_DEBUG(u8"IME text input received: %s (length: %d)", text.c_str(), text.size());
        
        if (_context && !text.is_empty())
        {
            // Ensure we're in the correct ImGui context
            ImGuiContext* prev_ctx = ImGui::GetCurrentContext();
            ImGui::SetCurrentContext(_context);
            
            // Add text to ImGui - convert from char8_t* to char*
            _context->IO.AddInputCharactersUTF8(reinterpret_cast<const char*>(text.c_str()));
            
            SKR_LOG_DEBUG(u8"Added text to ImGui IO");
            
            ImGui::SetCurrentContext(prev_ctx);
        }
    };
    
    // Composition text callback (optional, for showing text being typed)
    callbacks.on_composition_update = [this](const IMECompositionInfo& info) {
        // ImGui doesn't currently support showing composition text directly
        // But we could add custom rendering here if needed
        // For now, just log for debugging
        if (!info.text.is_empty())
        {
            SKR_LOG_TRACE(u8"IME composition: %s (cursor: %d)", info.text.c_str(), info.cursor_pos);
        }
    };
    
    // Candidates update callback (optional)
    callbacks.on_candidates_update = [this](const IMECandidateInfo& info) {
        // Could create custom candidate window
        // For now, let system IME show default candidate window
        SKR_LOG_TRACE(u8"IME candidates updated: %d candidates", info.total_candidates);
    };
    
    _ime->set_event_callbacks(callbacks);
    _ime_callbacks_set = true;
    
    SKR_LOG_DEBUG(u8"IME callbacks set successfully");
}

void ImGuiBackend::UpdateIMEState()
{
    if (!_ime || !_system_window)
        return;
        
    // Simple state sync: automatically manage IME based on WantTextInput
    bool imgui_wants_text = _context->IO.WantTextInput;
    
    // Use our own state tracking instead of querying IME every frame
    if (imgui_wants_text && !_ime_active_state)
    {
        // ImGui wants text input but we haven't activated IME yet
        _ime->start_text_input(_system_window);
        _ime_active_state = true;
        
        // Re-setup callbacks after starting IME (some implementations may require this)
        if (!_ime_callbacks_set)
        {
            SetupIMECallbacks();
        }
        
        SKR_LOG_DEBUG(u8"Starting IME text input (WantTextInput=true)");
    }
    else if (!imgui_wants_text && _ime_active_state)
    {
        // ImGui doesn't want text input but we have IME active
        _ime->stop_text_input();
        _ime_active_state = false;
        SKR_LOG_DEBUG(u8"Stopping IME text input (WantTextInput=false)");
    }
}

} // namespace skr