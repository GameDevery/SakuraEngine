#include "SkrGraphics/api.h"
#include "SkrTestFramework/framework.hpp"
#include <iostream>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include "windows.h" // IWYU pragma: keep
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HWND createWin32Window();
#elif defined(_MACOS)
    #include "SkrCore/platform/apple/macos/window.h"
#endif

template <ECGPUBackend backend>
class SwapChainCreation
{
protected:
    void Initialize() SKR_NOEXCEPT
    {
        SKR_DECLARE_ZERO(CGPUInstanceDescriptor, desc)
        desc.backend = backend;
        desc.enable_debug_layer = true;
        desc.enable_gpu_based_validation = true;
        instance = cgpu_create_instance(&desc);

        EXPECT_NE(instance, CGPU_NULLPTR);
        EXPECT_NE(instance, nullptr);

        uint32_t adapters_count = 0;
        cgpu_enum_adapters(instance, nullptr, &adapters_count);
        std::vector<CGPUAdapterId> adapters;
        adapters.resize(adapters_count);
        cgpu_enum_adapters(instance, adapters.data(), &adapters_count);
        adapter = adapters[0];

        CGPUQueueGroupDescriptor G = { CGPU_QUEUE_TYPE_GRAPHICS, 1 };
        SKR_DECLARE_ZERO(CGPUDeviceDescriptor, descriptor)
        descriptor.queue_groups = &G;
        descriptor.queue_group_count = 1;
        device = cgpu_create_device(adapter, &descriptor);
        EXPECT_NE(device, nullptr);
        EXPECT_NE(device, CGPU_NULLPTR);

#ifdef _WIN32
        hwnd = createWin32Window();
#elif defined(_MACOS)
        nswin = nswindow_create();
#endif
    }

    CGPUSwapChainId CreateSwapChainWithSurface(CGPUSurfaceId surface)
    {
        auto mainQueue = cgpu_get_queue(device, CGPU_QUEUE_TYPE_GRAPHICS, 0);
        SKR_DECLARE_ZERO(CGPUSwapChainDescriptor, descriptor)
        descriptor.present_queues = &mainQueue;
        descriptor.present_queues_count = 1;
        descriptor.surface = surface;
        descriptor.image_count = 3;
        descriptor.format = CGPU_FORMAT_R8G8B8A8_UNORM;
        descriptor.enable_vsync = true;

        auto swapchain = cgpu_create_swapchain(device, &descriptor);
        return swapchain;
    }

    SwapChainCreation() SKR_NOEXCEPT
    {
        Initialize();
    }

    ~SwapChainCreation() SKR_NOEXCEPT
    {
        cgpu_free_device(device);
        cgpu_free_instance(instance);
    }

    void test_all();

    CGPUInstanceId instance;
    CGPUAdapterId adapter;
    CGPUDeviceId device;
#ifdef _WIN32
    HWND hwnd;
#elif defined(_MACOS)
    void* nswin;
#endif
};

template <ECGPUBackend backend>
void SwapChainCreation<backend>::test_all()
{
#if defined(_WIN32) || defined(_WIN64)
SUBCASE("CreateFromHWND")
{
    auto surface = cgpu_surface_from_hwnd(device, hwnd);

    EXPECT_NE(surface, CGPU_NULLPTR);
    EXPECT_NE(surface, nullptr);

    auto swapchain = CreateSwapChainWithSurface(surface);

    EXPECT_NE(swapchain, CGPU_NULLPTR);
    EXPECT_NE(swapchain, nullptr);

    cgpu_free_swapchain(swapchain);
    cgpu_free_surface(device, surface);
}
#elif defined(__APPLE__)
    #define BACK_BUFFER_WIDTH 1280
    #define BACK_BUFFER_HEIGHT 720

SUBCASE("CreateFromNSView")
{
    auto ns_view = (struct NSView*)nswindow_get_content_view(nswin);
    auto surface = cgpu_surface_from_ns_view(device, (CGPUNSView*)ns_view);

    EXPECT_NE(surface, CGPU_NULLPTR);
    EXPECT_NE(surface, nullptr);

    auto swapchain = CreateSwapChainWithSurface(surface);

    EXPECT_NE(swapchain, CGPU_NULLPTR);
    EXPECT_NE(swapchain, nullptr);

    cgpu_free_swapchain(swapchain);
    cgpu_free_surface(device, surface);
}
#endif
}

#if defined(_WIN32) || defined(_WIN64)
LRESULT CALLBACK WindowProcedure(HWND window, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_DESTROY:
            std::cout << "\ndestroying window\n";
            PostQuitMessage(0);
            return 0L;
        case WM_LBUTTONDOWN:
            std::cout << "\nmouse left button down at (" << LOWORD(lp) << ',' << HIWORD(lp) << ")\n";
        default:
            return DefWindowProc(window, msg, wp, lp);
    }
}

HWND createWin32Window()
{
    // Register the window class.
    auto myclass = TEXT("myclass");
    WNDCLASSEX wndclass = {
        sizeof(WNDCLASSEX), CS_DBLCLKS,
        WindowProcedure,
        0, 0, GetModuleHandle(0), LoadIcon(0, IDI_APPLICATION),
        LoadCursor(0, IDC_ARROW), HBRUSH(COLOR_WINDOW + 1),
        0, myclass, LoadIcon(0, IDI_APPLICATION)
    };
    static bool bRegistered = RegisterClassEx(&wndclass);
    if (bRegistered)
    {
        HWND window = CreateWindowEx(0, myclass, TEXT("title"),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, GetModuleHandle(0), 0);
        if (window)
        {
            ShowWindow(window, SW_SHOWDEFAULT);
        }
        return window;
    }
    return CGPU_NULLPTR;
}
#endif

#ifdef CGPU_USE_D3D12
TEST_CASE_METHOD(SwapChainCreation<CGPU_BACKEND_D3D12>, "SwapChainCreation-d3d12")
{
    test_all();
}
#endif

#ifdef CGPU_USE_VULKAN
TEST_CASE_METHOD(SwapChainCreation<CGPU_BACKEND_VULKAN>, "SwapChainCreation-vulkan")
{
    test_all();
}
#endif