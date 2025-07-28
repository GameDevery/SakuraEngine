#pragma once
#include "SkrContainersDef/map.hpp"
#include "SkrContainersDef/sparse_vector.hpp"
#include "SkrRenderer/render_device.h"
#include "SkrSystem/system_app.h"

namespace skr
{
struct SKR_RENDERER_API RenderApp : public skr::SystemApp
{
public:
    RenderApp(SRenderDeviceId render_device);
    ~RenderApp();

    uint32_t open_window(const SystemWindowCreateInfo& create_info);
    void close_window(uint32_t idx);
    void close_all_windows();
    skr::SystemWindow* get_window(uint32_t idx);
    inline skr::SystemWindow* get_main_window() { return get_window(0); }
    CGPUSwapChainId get_swapchain(skr::SystemWindow* window);

    bool initialize(const char* backend) override;
    void shutdown() override;

protected:
    struct SwapchainManager : public skr::ISystemEventHandler
    {
    public:
        SwapchainManager(const RenderApp& app) SKR_NOEXCEPT;
        void handle_event(const SkrSystemEvent& event) SKR_NOEXCEPT override;
        CGPUSwapChainId get_swapchain(skr::SystemWindow* window);
        void destory_swapchain(skr::SystemWindow* window);
        void recreate_swapchain(skr::SystemWindow* window);
        struct SwapChain
        {
            CGPUSurfaceId _surface = nullptr;
            CGPUSwapChainId _swapchain = nullptr;
            float2 _size = float2(0, 0);
        };
    private:
        friend struct RenderApp;
        skr::Map<skr::SystemWindow*, SwapChain> _swapchains;
        const RenderApp& _app;
    } swapchain_manager;
    skr::SparseVector<skr::SystemWindow*> _windows;
    SRenderDeviceId _render_device;
};
} // namespace skr