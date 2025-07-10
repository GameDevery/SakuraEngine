#include "SkrSystem/system.h"
#include "SkrCore/log.h"
#include <thread>
#include <chrono>

// Test program to verify DPI alignment between Win32 and Cocoa backends
int main()
{
    // Create system app
    auto* app = skr::SystemApp::Create();
    if (!app) {
        SKR_LOG_ERROR(u8"Failed to create SystemApp");
        return -1;
    }
    
    // Get window manager
    auto* window_manager = app->get_window_manager();
    if (!window_manager) {
        SKR_LOG_ERROR(u8"Failed to get window manager");
        skr::SystemApp::Destroy(app);
        return -1;
    }
    
    // Test 1: Monitor information alignment
    SKR_LOG_INFO(u8"=== Monitor Information Test ===");
    uint32_t monitor_count = window_manager->get_monitor_count();
    SKR_LOG_INFO(u8"Found %d monitors", monitor_count);
    
    for (uint32_t i = 0; i < monitor_count; ++i) {
        auto* monitor = window_manager->get_monitor(i);
        if (monitor) {
            auto info = monitor->get_info();
            SKR_LOG_INFO(u8"Monitor %d: %s", i, info.name.c_str());
            SKR_LOG_INFO(u8"  Position: (%d, %d) logical pixels", info.position.x, info.position.y);
            SKR_LOG_INFO(u8"  Size: %dx%d logical pixels", info.size.x, info.size.y);
            SKR_LOG_INFO(u8"  DPI Scale: %.2f", info.dpi_scale);
            SKR_LOG_INFO(u8"  Physical Size: %dx%d pixels", 
                (uint32_t)(info.size.x * info.dpi_scale), 
                (uint32_t)(info.size.y * info.dpi_scale));
            SKR_LOG_INFO(u8"  Work Area: (%d, %d) %dx%d logical pixels", 
                info.work_area_pos.x, info.work_area_pos.y,
                info.work_area_size.x, info.work_area_size.y);
            SKR_LOG_INFO(u8"  Primary: %s", info.is_primary ? "Yes" : "No");
        }
    }
    
    // Test 2: Window creation and positioning
    SKR_LOG_INFO(u8"\n=== Window Creation Test ===");
    
    skr::SystemWindowCreateInfo create_info;
    create_info.title = u8"DPI Test Window";
    create_info.size = {800, 600};  // Logical pixels
    create_info.pos = {100, 100};   // Logical pixels
    create_info.is_resizable = true;
    
    auto* window = window_manager->create_window(create_info);
    if (!window) {
        SKR_LOG_ERROR(u8"Failed to create window");
        skr::SystemApp::Destroy(app);
        return -1;
    }
    
    window->show();
    
    // Give window time to be created
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Test window properties
    SKR_LOG_INFO(u8"Created window at logical position (%d, %d)", 
        create_info.pos->x, create_info.pos->y);
    
    auto actual_pos = window->get_position();
    auto actual_size = window->get_size();
    auto physical_size = window->get_physical_size();
    float pixel_ratio = window->get_pixel_ratio();
    
    SKR_LOG_INFO(u8"Actual position: (%d, %d) logical pixels", actual_pos.x, actual_pos.y);
    SKR_LOG_INFO(u8"Actual size: %dx%d logical pixels", actual_size.x, actual_size.y);
    SKR_LOG_INFO(u8"Physical size: %dx%d pixels", physical_size.x, physical_size.y);
    SKR_LOG_INFO(u8"Pixel ratio: %.2f", pixel_ratio);
    
    // Test 3: Move window to different positions
    SKR_LOG_INFO(u8"\n=== Window Movement Test ===");
    
    struct TestPosition {
        int32_t x, y;
        const char8_t* description;
    };
    
    TestPosition test_positions[] = {
        {0, 0, u8"Top-left corner"},
        {200, 200, u8"Center area"},
        {-50, 100, u8"Partially off-screen left"},
        {100, -50, u8"Partially off-screen top"}
    };
    
    for (const auto& test_pos : test_positions) {
        window->set_position(test_pos.x, test_pos.y);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto new_pos = window->get_position();
        SKR_LOG_INFO(u8"Set to (%d, %d) - %s", test_pos.x, test_pos.y, test_pos.description);
        SKR_LOG_INFO(u8"  Got (%d, %d)", new_pos.x, new_pos.y);
        
        if (new_pos.x != test_pos.x || new_pos.y != test_pos.y) {
            SKR_LOG_WARN(u8"  Position mismatch!");
        }
    }
    
    // Test 4: Window sizing
    SKR_LOG_INFO(u8"\n=== Window Sizing Test ===");
    
    struct TestSize {
        uint32_t width, height;
        const char8_t* description;
    };
    
    TestSize test_sizes[] = {
        {400, 300, u8"Small size"},
        {1024, 768, u8"Standard size"},
        {1920, 1080, u8"Full HD size"}
    };
    
    for (const auto& test_size : test_sizes) {
        window->set_size(test_size.width, test_size.height);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto new_size = window->get_size();
        auto new_physical = window->get_physical_size();
        
        SKR_LOG_INFO(u8"Set to %dx%d - %s", test_size.width, test_size.height, test_size.description);
        SKR_LOG_INFO(u8"  Logical: %dx%d", new_size.x, new_size.y);
        SKR_LOG_INFO(u8"  Physical: %dx%d", new_physical.x, new_physical.y);
        SKR_LOG_INFO(u8"  Ratio: %.2f", (float)new_physical.x / new_size.x);
        
        if (new_size.x != test_size.width || new_size.y != test_size.height) {
            SKR_LOG_WARN(u8"  Size mismatch!");
        }
    }
    
    // Test 5: Multi-monitor positioning
    if (monitor_count > 1) {
        SKR_LOG_INFO(u8"\n=== Multi-Monitor Test ===");
        
        // Try positioning window on secondary monitor
        auto* secondary_monitor = window_manager->get_monitor(1);
        if (secondary_monitor) {
            auto monitor_info = secondary_monitor->get_info();
            
            // Position window at center of secondary monitor
            int32_t center_x = monitor_info.position.x + monitor_info.size.x / 2 - 400;
            int32_t center_y = monitor_info.position.y + monitor_info.size.y / 2 - 300;
            
            window->set_position(center_x, center_y);
            window->set_size(800, 600);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            auto window_monitor = window_manager->get_monitor_from_window(window);
            if (window_monitor == secondary_monitor) {
                SKR_LOG_INFO(u8"Successfully moved window to secondary monitor");
            } else {
                SKR_LOG_WARN(u8"Window not on expected monitor");
            }
            
            auto final_pos = window->get_position();
            SKR_LOG_INFO(u8"Final position on secondary monitor: (%d, %d)", 
                final_pos.x, final_pos.y);
        }
    }
    
    // Keep window open for manual inspection
    SKR_LOG_INFO(u8"\n=== Press Ctrl+C to exit ===");
    
    while (true) {
        app->get_event_queue()->pump_messages();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // Cleanup
    window_manager->destroy_window(window);
    skr::SystemApp::Destroy(app);
    
    return 0;
}