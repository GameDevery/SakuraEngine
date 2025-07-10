#include "SkrSystem/system.h"
#include "SkrCore/log.h"
#include <thread>
#include <chrono>
#include <algorithm>
#include <cmath>

// Test program to verify DPI alignment between Win32 and Cocoa backends
// 
// This comprehensive test suite validates:
// 1. Monitor information consistency across platforms
// 2. Window positioning and sizing in logical pixels
// 3. DPI scale factor reporting and physical pixel calculations
// 4. Dynamic DPI changes when moving windows between monitors
// 5. Window creation on high-DPI monitors
// 6. Coordinate system conversion accuracy
// 7. Multi-monitor boundary behavior
// 8. Fullscreen mode DPI handling
// 9. Window size constraints
//
// Key concepts tested:
// - Logical pixels (device-independent) vs Physical pixels (actual screen pixels)
// - DPI scale factor = Physical pixels / Logical pixels
// - Win32 Per-Monitor DPI Aware V2 behavior
// - macOS backing scale factor behavior
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
    // This test verifies that monitor information is reported consistently
    // across Win32 and Cocoa backends, including:
    // - Monitor enumeration order
    // - Logical position and size (in DIPs/points)
    // - DPI scale factors matching OS settings
    // - Work area calculations (excluding taskbar/dock)
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
            
            // Validate DPI scale is reasonable
            if (info.dpi_scale < 0.5f || info.dpi_scale > 10.0f) {
                SKR_LOG_ERROR(u8"  Invalid DPI scale %.2f - expected range [0.5, 10.0]", info.dpi_scale);
            }
            
            // Validate size is non-zero
            if (info.size.x == 0 || info.size.y == 0) {
                SKR_LOG_ERROR(u8"  Invalid monitor size %dx%d - dimensions must be > 0", info.size.x, info.size.y);
            }
            
            SKR_LOG_INFO(u8"  Physical Size: %dx%d pixels", 
                (uint32_t)(info.size.x * info.dpi_scale), 
                (uint32_t)(info.size.y * info.dpi_scale));
            SKR_LOG_INFO(u8"  Work Area: (%d, %d) %dx%d logical pixels", 
                info.work_area_pos.x, info.work_area_pos.y,
                info.work_area_size.x, info.work_area_size.y);
                
            // Validate work area is within monitor bounds
            if (info.work_area_size.x > info.size.x || info.work_area_size.y > info.size.y) {
                SKR_LOG_ERROR(u8"  Work area size (%dx%d) exceeds monitor size (%dx%d)", 
                    info.work_area_size.x, info.work_area_size.y, info.size.x, info.size.y);
            }
            
            SKR_LOG_INFO(u8"  Primary: %s", info.is_primary ? "Yes" : "No");
        } else {
            SKR_LOG_ERROR(u8"Failed to get monitor %d", i);
        }
    }
    
    // Test 2: Window creation and positioning
    // This test validates that windows are created with correct:
    // - Initial position in logical coordinates
    // - Size in logical pixels (device-independent)
    // - Proper DPI awareness from creation
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
    
    // Validate initial window properties
    if (std::abs(actual_pos.x - (int)create_info.pos->x) > 50 || std::abs(actual_pos.y - (int)create_info.pos->y) > 50) {
        SKR_LOG_ERROR(u8"Window position mismatch: expected (%d, %d), got (%d, %d)", 
            create_info.pos->x, create_info.pos->y, actual_pos.x, actual_pos.y);
    }
    
    if (actual_size.x != create_info.size.x || actual_size.y != create_info.size.y) {
        SKR_LOG_ERROR(u8"Window size mismatch: expected %dx%d, got %dx%d", 
            create_info.size.x, create_info.size.y, actual_size.x, actual_size.y);
    }
    
    if (pixel_ratio < 0.5f || pixel_ratio > 10.0f) {
        SKR_LOG_ERROR(u8"Invalid pixel ratio %.2f - expected range [0.5, 10.0]", pixel_ratio);
    }
    
    // Validate physical size calculation consistency
    uint32_t expected_physical_x = (uint32_t)(actual_size.x * pixel_ratio);
    uint32_t expected_physical_y = (uint32_t)(actual_size.y * pixel_ratio);
    if (std::abs((int32_t)physical_size.x - (int32_t)expected_physical_x) > 2 || 
        std::abs((int32_t)physical_size.y - (int32_t)expected_physical_y) > 2) {
        SKR_LOG_ERROR(u8"Physical size calculation inconsistent: expected %dx%d, got %dx%d", 
            expected_physical_x, expected_physical_y, physical_size.x, physical_size.y);
    }
    
    // Test 3: Move window to different positions
    // This test verifies window positioning behavior:
    // - Accurate placement at specified logical coordinates
    // - Handling of edge cases (negative coordinates)
    // - Platform-specific window manager constraints
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
        
        if (std::abs(new_pos.x - test_pos.x) > 10 || std::abs(new_pos.y - test_pos.y) > 10) {
            SKR_LOG_ERROR(u8"  Position mismatch: expected (%d, %d), got (%d, %d), delta (%d, %d)", 
                test_pos.x, test_pos.y, new_pos.x, new_pos.y, 
                new_pos.x - test_pos.x, new_pos.y - test_pos.y);
        }
    }
    
    // Test 4: Window sizing
    // This test validates window sizing behavior:
    // - Size changes in logical pixels
    // - Automatic physical size calculation
    // - Consistent pixel ratio maintenance
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
            SKR_LOG_ERROR(u8"  Size mismatch: expected %dx%d, got %dx%d", 
                test_size.width, test_size.height, new_size.x, new_size.y);
        }
        
        // Validate physical/logical size consistency
        float calculated_ratio = (float)new_physical.x / new_size.x;
        if (std::abs(calculated_ratio - pixel_ratio) > 0.1f) {
            SKR_LOG_ERROR(u8"  Pixel ratio inconsistency: window reports %.2f, calculated %.2f", 
                pixel_ratio, calculated_ratio);
        }
    }
    
    // Test 5: Multi-monitor positioning
    // This test validates multi-monitor support:
    // - Window placement on specific monitors
    // - Monitor detection from window position
    // - Coordinate space continuity across monitors
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
                
                // Validate DPI matches monitor
                float window_dpi = window->get_pixel_ratio();
                if (std::abs(window_dpi - monitor_info.dpi_scale) > 0.1f) {
                    SKR_LOG_ERROR(u8"Window DPI %.2f doesn't match monitor DPI %.2f", 
                        window_dpi, monitor_info.dpi_scale);
                }
            } else {
                SKR_LOG_ERROR(u8"Window not on expected monitor - monitor detection failed");
                if (window_monitor) {
                    auto actual_info = window_monitor->get_info();
                    SKR_LOG_ERROR(u8"  Window is on: %s", actual_info.name.c_str());
                    SKR_LOG_ERROR(u8"  Expected: %s", monitor_info.name.c_str());
                } else {
                    SKR_LOG_ERROR(u8"  get_monitor_from_window returned null");
                }
            }
            
            auto final_pos = window->get_position();
            SKR_LOG_INFO(u8"Final position on secondary monitor: (%d, %d)", 
                final_pos.x, final_pos.y);
        }
    }
    
    // Test 6: DPI Change Detection Test
    // This critical test verifies dynamic DPI handling:
    // - Real-time DPI scale updates when crossing monitor boundaries
    // - Proper WM_DPICHANGED handling on Win32
    // - NSWindowDidChangeBackingProperties on macOS
    // - Physical size recalculation accuracy
    SKR_LOG_INFO(u8"\n=== DPI Change Detection Test ===");
    SKR_LOG_INFO(u8"Current window DPI scale: %.2f", window->get_pixel_ratio());
    if (monitor_count > 1) {
        SKR_LOG_INFO(u8"Move the window between monitors with different DPI scales");
        SKR_LOG_INFO(u8"Observing DPI changes for 10 seconds...");
        
        auto start_time = std::chrono::steady_clock::now();
        float last_pixel_ratio = window->get_pixel_ratio();
        
        while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(10)) {
            app->get_event_queue()->pump_messages();
            
            float current_pixel_ratio = window->get_pixel_ratio();
            if (std::abs(current_pixel_ratio - last_pixel_ratio) > 0.01f) {
                auto current_monitor = window_manager->get_monitor_from_window(window);
                auto monitor_info = current_monitor ? current_monitor->get_info() : skr::SystemMonitorInfo{};
                
                SKR_LOG_INFO(u8"DPI Changed: %.2f -> %.2f (Monitor: %s)", 
                    last_pixel_ratio, current_pixel_ratio, 
                    monitor_info.name.c_str());
                
                // Verify physical size updates correctly
                auto logical_size = window->get_size();
                auto physical_size = window->get_physical_size();
                float calculated_ratio = (float)physical_size.x / logical_size.x;
                
                SKR_LOG_INFO(u8"  Logical size: %dx%d", logical_size.x, logical_size.y);
                SKR_LOG_INFO(u8"  Physical size: %dx%d", physical_size.x, physical_size.y);
                SKR_LOG_INFO(u8"  Calculated ratio: %.2f", calculated_ratio);
                
                // Validate physical size calculation
                if (std::abs(calculated_ratio - current_pixel_ratio) > 0.1f) {
                    SKR_LOG_ERROR(u8"  Physical size calculation error: pixel ratio %.2f but calculated %.2f", 
                        current_pixel_ratio, calculated_ratio);
                }
                
                // Validate DPI matches monitor
                if (current_monitor && std::abs(current_pixel_ratio - monitor_info.dpi_scale) > 0.1f) {
                    SKR_LOG_ERROR(u8"  Window DPI %.2f doesn't match monitor DPI %.2f", 
                        current_pixel_ratio, monitor_info.dpi_scale);
                }
                
                last_pixel_ratio = current_pixel_ratio;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } else {
        SKR_LOG_INFO(u8"Single monitor detected, skipping DPI change test");
    }
    
    // Test 7: Window Creation on High DPI Monitor
    // This test ensures windows created on high-DPI monitors:
    // - Initialize with correct DPI scale
    // - Don't require post-creation DPI adjustment
    // - Handle Win32's initial DPI query correctly
    SKR_LOG_INFO(u8"\n=== Window Creation DPI Test ===");
    
    // Find the monitor with the highest DPI
    skr::SystemMonitor* high_dpi_monitor = nullptr;
    float max_dpi_scale = 0.0f;
    
    for (uint32_t i = 0; i < monitor_count; ++i) {
        auto* monitor = window_manager->get_monitor(i);
        if (monitor) {
            auto info = monitor->get_info();
            if (info.dpi_scale > max_dpi_scale) {
                max_dpi_scale = info.dpi_scale;
                high_dpi_monitor = monitor;
            }
        }
    }
    
    if (high_dpi_monitor && max_dpi_scale > 1.0f) {
        auto monitor_info = high_dpi_monitor->get_info();
        SKR_LOG_INFO(u8"Creating window on high DPI monitor: %s (DPI scale: %.2f)",
            monitor_info.name.c_str(), monitor_info.dpi_scale);
        
        // Position new window at center of high DPI monitor
        skr::SystemWindowCreateInfo hdpi_create_info;
        hdpi_create_info.title = u8"High DPI Test Window";
        hdpi_create_info.size = {600, 400};
        hdpi_create_info.pos = {
            monitor_info.position.x + monitor_info.size.x / 2 - 300,
            monitor_info.position.y + monitor_info.size.y / 2 - 200
        };
        
        auto* hdpi_window = window_manager->create_window(hdpi_create_info);
        if (hdpi_window) {
            hdpi_window->show();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            // Verify initial DPI is correct
            float initial_pixel_ratio = hdpi_window->get_pixel_ratio();
            SKR_LOG_INFO(u8"New window pixel ratio: %.2f (expected: %.2f)",
                initial_pixel_ratio, monitor_info.dpi_scale);
            
            if (std::abs(initial_pixel_ratio - monitor_info.dpi_scale) > 0.1f) {
                SKR_LOG_ERROR(u8"Initial DPI mismatch: window %.2f vs monitor %.2f (diff: %.3f)", 
                    initial_pixel_ratio, monitor_info.dpi_scale, 
                    std::abs(initial_pixel_ratio - monitor_info.dpi_scale));
            } else {
                SKR_LOG_INFO(u8"✓ Initial DPI correctly matches monitor");
            }
            
            window_manager->destroy_window(hdpi_window);
        }
    } else {
        SKR_LOG_INFO(u8"No high DPI monitor found, skipping test");
    }
    
    // Test 8: Coordinate System Consistency Test
    // This test validates the mathematical relationship:
    // Physical pixels = Logical pixels × DPI scale
    // Critical for ensuring Win32 and Cocoa report consistent values
    SKR_LOG_INFO(u8"\n=== Coordinate System Consistency Test ===");
    
    // Test logical to physical coordinate conversion
    struct CoordTest {
        uint32_t logical_size;
        float dpi_scale;
    };
    
    CoordTest coord_tests[] = {
        {100, 1.0f},
        {100, 1.25f},
        {100, 1.5f},
        {100, 2.0f},
        {200, 1.5f},
        {300, 2.0f}
    };
    
    for (const auto& test : coord_tests) {
        uint32_t expected_physical = (uint32_t)(test.logical_size * test.dpi_scale);
        SKR_LOG_INFO(u8"Logical: %u, DPI Scale: %.2f -> Expected Physical: %u",
            test.logical_size, test.dpi_scale, expected_physical);
        
        // Note: Actual window creation and verification would depend on
        // having monitors with these specific DPI scales
    }
    
    // Test 9: Window Bounds Across Multiple Monitors
    // This test verifies behavior when windows span monitors:
    // - DPI assignment based on majority area
    // - Win32: window assigned to monitor containing center point
    // - macOS: window assigned to monitor with >50% of window area
    if (monitor_count > 1) {
        SKR_LOG_INFO(u8"\n=== Multi-Monitor Boundary Test ===");
        
        auto* monitor0 = window_manager->get_monitor(0);
        auto* monitor1 = window_manager->get_monitor(1);
        
        if (monitor0 && monitor1) {
            auto info0 = monitor0->get_info();
            auto info1 = monitor1->get_info();
            
            // Position window to span across both monitors
            int32_t boundary_x = info0.position.x + info0.size.x - 200;
            int32_t boundary_y = std::min(info0.position.y, info1.position.y) + 100;
            
            window->set_position(boundary_x, boundary_y);
            window->set_size(400, 300);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            SKR_LOG_INFO(u8"Window spanning monitors:");
            SKR_LOG_INFO(u8"  Monitor 0: %s (DPI: %.2f)", 
                info0.name.c_str(), info0.dpi_scale);
            SKR_LOG_INFO(u8"  Monitor 1: %s (DPI: %.2f)", 
                info1.name.c_str(), info1.dpi_scale);
            
            auto assigned_monitor = window_manager->get_monitor_from_window(window);
            if (assigned_monitor) {
                auto assigned_info = assigned_monitor->get_info();
                SKR_LOG_INFO(u8"  Window assigned to: %s", assigned_info.name.c_str());
                SKR_LOG_INFO(u8"  Window DPI scale: %.2f", window->get_pixel_ratio());
            }
        }
    }
    
    // Test 10: Fullscreen Mode DPI Handling
    // This test validates fullscreen behavior across monitors:
    // - DPI scale matches target monitor
    // - Logical size matches monitor logical size
    // - Physical size matches native resolution
    SKR_LOG_INFO(u8"\n=== Fullscreen DPI Test ===");
    
    // Test fullscreen on each monitor
    for (uint32_t i = 0; i < monitor_count && i < 2; ++i) {
        auto* monitor = window_manager->get_monitor(i);
        if (monitor) {
            auto info = monitor->get_info();
            SKR_LOG_INFO(u8"Testing fullscreen on monitor %u: %s (DPI: %.2f)",
                i, info.name.c_str(), info.dpi_scale);
            
            // Enter fullscreen
            window->set_fullscreen(true, monitor);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            
            // Check DPI in fullscreen
            float fullscreen_dpi = window->get_pixel_ratio();
            auto fullscreen_size = window->get_size();
            auto fullscreen_physical = window->get_physical_size();
            
            SKR_LOG_INFO(u8"  Fullscreen DPI: %.2f", fullscreen_dpi);
            SKR_LOG_INFO(u8"  Fullscreen logical size: %dx%d", 
                fullscreen_size.x, fullscreen_size.y);
            SKR_LOG_INFO(u8"  Fullscreen physical size: %dx%d", 
                fullscreen_physical.x, fullscreen_physical.y);
                
            // Validate fullscreen DPI matches monitor
            if (std::abs(fullscreen_dpi - info.dpi_scale) > 0.1f) {
                SKR_LOG_ERROR(u8"  Fullscreen DPI %.2f doesn't match monitor DPI %.2f", 
                    fullscreen_dpi, info.dpi_scale);
            }
            
            // Validate fullscreen logical size matches monitor logical size
            if (std::abs((int32_t)fullscreen_size.x - (int32_t)info.size.x) > 10 || 
                std::abs((int32_t)fullscreen_size.y - (int32_t)info.size.y) > 10) {
                SKR_LOG_ERROR(u8"  Fullscreen logical size %dx%d doesn't match monitor size %dx%d", 
                    fullscreen_size.x, fullscreen_size.y, info.size.x, info.size.y);
            }
            
            // Validate physical size calculation
            uint32_t expected_phys_x = (uint32_t)(fullscreen_size.x * fullscreen_dpi);
            uint32_t expected_phys_y = (uint32_t)(fullscreen_size.y * fullscreen_dpi);
            if (std::abs((int32_t)fullscreen_physical.x - (int32_t)expected_phys_x) > 10 || 
                std::abs((int32_t)fullscreen_physical.y - (int32_t)expected_phys_y) > 10) {
                SKR_LOG_ERROR(u8"  Fullscreen physical size calculation error: expected %dx%d, got %dx%d", 
                    expected_phys_x, expected_phys_y, fullscreen_physical.x, fullscreen_physical.y);
            }
            
            // Exit fullscreen
            window->set_fullscreen(false);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
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