#include "SkrCore/module/module_manager.hpp"
#include <SkrCore/crash.h>
#include <SkrCore/log.h>
#include <SkrCore/dirty.hpp>
#include <SkrOS/filesystem.hpp>
#include <SkrSystem/window.h>
#include <SkrSystem/event.h>
#include <SkrSystem/system_app.h>

struct DummyEventHandler : public skr::ISystemEventHandler
{
    ~DummyEventHandler() SKR_NOEXCEPT = default;
    void handle_event(const SkrSystemEvent& event) SKR_NOEXCEPT override
    {
        // Handle events here if needed
        switch (event.type)
        {
        case SKR_SYSTEM_EVENT_WINDOW_CLOSE_REQUESTED:
        case SKR_SYSTEM_EVENT_QUIT:
            _want_exit.trigger();
            break;
        default:
            // Handle other events if necessary
            break;
        }
    }
    const skr::Trigger& want_exit() const SKR_NOEXCEPT { return _want_exit; }

private:
    skr::Trigger _want_exit = {};
};

struct PlainWindowSampleModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;
};

PlainWindowSampleModule g_plain_window_sample_module;
IMPLEMENT_DYNAMIC_MODULE(PlainWindowSampleModule, SystemSample_PlainWindow);
void PlainWindowSampleModule::on_load(int argc, char8_t** argv)
{
    SKR_LOG_INFO(u8"Plain Window Sample Module Loaded");
}

void PlainWindowSampleModule::on_unload()
{
    SKR_LOG_INFO(u8"Plain Window Sample Module Unloaded");
}

int PlainWindowSampleModule::main_module_exec(int argc, char8_t** argv)
{
    // Step 01: Create a system app instance
    auto* app = SkrNew<skr::SystemApp>();
    if (!app)
    {
        SKR_LOG_ERROR(u8"Failed to create SystemApp");
        return -1;
    }
    app->initialize("SDL3"); // Use SDL3 backend for this sample

    // Step 02: Get the window manager
    auto* window_manager = app->get_window_manager();
    if (!window_manager)
    {
        SKR_LOG_ERROR(u8"Failed to get window manager");
        SkrDelete(app);
        return -1;
    }

    // Step 03: Check Available Monitors
    uint32_t monitor_count = window_manager->get_monitor_count();
    if (monitor_count == 0)
    {
        SKR_LOG_ERROR(u8"No monitors found");
        SkrDelete(app);
        return -1;
    }

    // Step 04: Create a Plain Window
    skr::SystemWindowCreateInfo create_info;
    create_info.title = u8"Plain Window Sample";
    create_info.size = { 800, 600 }; // Logical pixels
    create_info.pos = { 100, 100 };  // Logical pixels
    create_info.is_resizable = true;

    auto* window = window_manager->create_window(create_info);

    if (!window)
    {
        SKR_LOG_ERROR(u8"Failed to create window");
        SkrDelete(app);
        return -1;
    }

    // Step 05: Show the Window
    window->show();

    // Give window time to be created
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // Test window properties
    SKR_LOG_INFO(u8"Created window at logical position (%d, %d) with size (%d, %d)",
        create_info.pos->x,
        create_info.pos->y,
        create_info.size.x,
        create_info.size.y);

    // Step 06: Create and Register Event Handler
    auto* event_handler = SkrNew<DummyEventHandler>();
    if (!event_handler)
    {
        SKR_LOG_ERROR(u8"Failed to create event handler");
        window_manager->destroy_window(window);
        SkrDelete(app);
        return -1;
    }
    app->get_event_queue()->add_handler(event_handler);

    // Step 06: Event Loop
    SKR_LOG_INFO(u8"\n=== Press Ctrl+C to exit ===");
    while (!event_handler->want_exit().comsume())
    {
        app->get_event_queue()->pump_messages();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // Step 07: Cleanup
    window_manager->destroy_window(window);
    SkrDelete(app);
    SKR_LOG_INFO(u8"Plain Window Sample completed successfully.");
    return 0;
}

// Sample for Plain Window Creation
int main(int argc, char* argv[])
{
    auto moduleManager = skr_get_module_manager();
    auto root = skr::fs::Directory::current();
    {
        FrameMark;
        SkrZoneScopedN("Initialize");
        moduleManager->mount(root.to_string().c_str());
        moduleManager->make_module_graph(u8"SystemSample_PlainWindow", true);
        moduleManager->init_module_graph(argc, argv);
        moduleManager->destroy_module_graph();
    }
    return 0;
}