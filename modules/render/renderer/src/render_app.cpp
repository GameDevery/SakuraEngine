#include "SkrRenderer/render_app.hpp"

namespace skr {

RenderApp::RenderApp(SRenderDeviceId render_device)
    : _render_device(render_device), swapchain_manager(*this)
{

}

RenderApp::~RenderApp()
{

}

uint32_t RenderApp::open_window(const SystemWindowCreateInfo& create_info)
{
    auto new_window = window_manager->create_window(create_info);
    swapchain_manager.recreate_swapchain(new_window);
    auto index = _windows.add(new_window).index();
    return index;
}

void RenderApp::close_window(uint32_t idx)
{
    if (auto to_close = get_window(idx))
    {
        _windows.remove(to_close);
        window_manager->destroy_window(to_close);
    }
}

void RenderApp::close_all_windows()
{
    for (auto window : _windows)
    {
        window_manager->destroy_all_windows();
    }
    _windows.clear();
}

skr::SystemWindow* RenderApp::get_window(uint32_t idx)
{
    return _windows.at(idx);
}

CGPUSwapChainId RenderApp::get_swapchain(skr::SystemWindow* window)
{
    return swapchain_manager.get_swapchain(window);
}

bool RenderApp::initialize(const char* backend)
{
    if (!SystemApp::initialize(backend))
        return false;
    get_event_queue()->add_handler(&swapchain_manager);
    return true;
}

void RenderApp::shutdown()
{
    SystemApp::shutdown();
    get_event_queue()->remove_handler(&swapchain_manager);
}

RenderApp::SwapchainManager::SwapchainManager(const RenderApp& app) SKR_NOEXCEPT
    : _app(app)
{
            
}

void RenderApp::SwapchainManager::handle_event(const SkrSystemEvent& event) SKR_NOEXCEPT
{
    if (event.window.type == SKR_SYSTEM_EVENT_WINDOW_RESIZED)
    {
        auto wm = _app.get_window_manager();
        recreate_swapchain(wm->get_window_by_native_handle(event.window.window_native_handle));
    }
    else if (event.window.type == SKR_SYSTEM_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        auto wm = _app.get_window_manager();
        destory_swapchain(wm->get_window_by_native_handle(event.window.window_native_handle));
    }
}

CGPUSwapChainId RenderApp::SwapchainManager::get_swapchain(skr::SystemWindow* window)
{
    auto found = _swapchains.find(window);
    return found ? found.value()._swapchain : nullptr;
}

void RenderApp::SwapchainManager::destory_swapchain(skr::SystemWindow* window)
{
    auto cgpu_device = _app._render_device->get_cgpu_device();
    auto gfx_queue = _app._render_device->get_gfx_queue();
    if (auto s = _swapchains.find(window))
    {
        cgpu_wait_queue_idle(gfx_queue);
        cgpu_free_swapchain(s.value()._swapchain);
        cgpu_free_surface(cgpu_device, s.value()._surface);
        _swapchains.remove_at(s.index());
    }
}

void RenderApp::SwapchainManager::recreate_swapchain(skr::SystemWindow* window)
{
    destory_swapchain(window);

    auto cgpu_device = _app._render_device->get_cgpu_device();
    auto gfx_queue = _app._render_device->get_gfx_queue();
    const auto phys_size = window->get_physical_size();
    auto surface = cgpu_surface_from_native_view(_app._render_device->get_cgpu_device(), window->get_native_view());
    CGPUSwapChainDescriptor chain_desc = {};
    chain_desc.surface = surface;
    chain_desc.present_queues = &gfx_queue;
    chain_desc.present_queues_count = 1;
    chain_desc.width = phys_size.x; 
    chain_desc.height = phys_size.y;
    chain_desc.image_count = 3;
    chain_desc.enable_vsync = true;
    chain_desc.format = CGPU_FORMAT_R8G8B8A8_UNORM;
    auto swapchain = cgpu_create_swapchain(cgpu_device, &chain_desc);
    _swapchains.add(window, SwapChain{ 
        ._surface = surface, 
        ._swapchain = swapchain,
        ._size =  phys_size 
    });
}

} // namespace skr