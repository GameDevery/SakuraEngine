#include "SkrCore/module/module_manager.hpp"
#include <SkrOS/filesystem.hpp>

int main(int argc, char* argv[]) {
    auto moduleManager = skr_get_module_manager();
    std::error_code ec = {};
    auto root = skr::filesystem::current_path(ec);
    {
        FrameMark;
        SkrZoneScopedN("Initialize");
        moduleManager->mount(root.u8string().c_str());
        moduleManager->make_module_graph(u8"SceneSample_Simple", true);
        moduleManager->init_module_graph(argc, argv);
        moduleManager->destroy_module_graph();
    }
    return 0;
}