#include "SkrSystem/system_app.h"
#include "SkrCore/log.h"
#include "SkrCore/memory/memory.h"

int main(int argc, char* argv[])
{
    // 初始化日志系统
    skr_log_set_level(SKR_LOG_LEVEL_INFO);
    
    // 创建系统应用程序
    auto* app = skr::SystemApp::Create("Win32");
    if (!app)
    {
        SKR_LOG_ERROR(u8"Failed to create SystemApp");
        return -1;
    }
    
    // 获取窗口管理器
    auto* window_manager = app->get_window_manager();
    if (!window_manager)
    {
        SKR_LOG_ERROR(u8"No window manager available");
        skr::SystemApp::Destroy(app);
        return -1;
    }
    
    // 创建窗口
    skr::SystemWindowCreateInfo window_info;
    window_info.title = u8"SakuraEngine - Simple Window Example";
    window_info.size = {1280, 720};
    window_info.is_resizable = true;
    
    auto* window = window_manager->create_window(window_info);
    if (!window)
    {
        SKR_LOG_ERROR(u8"Failed to create window");
        skr::SystemApp::Destroy(app);
        return -1;
    }
    
    // 显示窗口
    window->show();
    window->focus();
    
    SKR_LOG_INFO(u8"Window created successfully");
    SKR_LOG_INFO(u8"Window size: %dx%d", window->get_size().x, window->get_size().y);
    SKR_LOG_INFO(u8"Pixel ratio: %f", window->get_pixel_ratio());
    
    // 获取事件队列
    auto* event_queue = app->get_event_queue();
    
    // 主循环
    bool running = true;
    while (running)
    {
        // 处理事件
        event_queue->pump_messages();
        
        // 获取平台事件源并处理事件
        auto* platform_source = app->get_platform_event_source();
        SkrSystemEvent event;
        
        while (platform_source->poll_event(event))
        {
            switch (event.type)
            {
                case SKR_SYSTEM_EVENT_QUIT:
                    SKR_LOG_INFO(u8"Quit event received");
                    running = false;
                    break;
                    
                case SKR_SYSTEM_EVENT_WINDOW_CLOSE_REQUESTED:
                    SKR_LOG_INFO(u8"Window close requested");
                    running = false;
                    break;
                    
                case SKR_SYSTEM_EVENT_WINDOW_RESIZED:
                {
                    auto& window_event = event.window;
                    SKR_LOG_INFO(u8"Window resized to %dx%d", window_event.x, window_event.y);
                    break;
                }
                
                case SKR_SYSTEM_EVENT_KEY_DOWN:
                {
                    auto& key_event = event.key;
                    if (key_event.keycode == KEY_CODE_Esc)
                    {
                        SKR_LOG_INFO(u8"ESC key pressed, exiting");
                        running = false;
                    }
                    break;
                }
                
                case SKR_SYSTEM_EVENT_MOUSE_BUTTON_DOWN:
                {
                    auto& mouse_event = event.mouse;
                    SKR_LOG_INFO(u8"Mouse button pressed at (%d, %d)", mouse_event.x, mouse_event.y);
                    break;
                }
                
                default:
                    break;
            }
        }
        
        // 可以在这里添加渲染或其他逻辑
        // 现在只是一个空循环，展示基本的窗口和事件处理
    }
    
    SKR_LOG_INFO(u8"Shutting down...");
    
    // 清理
    window_manager->destroy_window(window);
    skr::SystemApp::Destroy(app);
    
    return 0;
}