#include "SkrCore/log.h"
#include "SkrBase/misc/make_zeroed.hpp"
#include "SkrCore/platform/vfs.h"
#include "SkrRT/io/ram_io.hpp"
#include "SkrToolCore/project/project.hpp"
#include "SkrSerde/json_serde.hpp"

namespace skd
{

// void SProject::SetAssetVFS(skr_vfs_t* asset_vfs)
// {
//     this->asset_vfs = asset_vfs;
// }

// void SProject::SetResourceVFS(skr_vfs_t* resource_vfs)
// {
//     this->resource_vfs = resource_vfs;
// }

// void SProject::SetRAMService(skr::io::IRAMService* service)
// {
//     this->ram_service = service;
// }

skr_vfs_t* SProject::GetAssetVFS() const
{
    return asset_vfs;
}

skr_vfs_t* SProject::GetResourceVFS() const
{
    return resource_vfs;
}

skr_vfs_t* SProject::GetDependencyVFS() const
{
    return dependency_vfs;
}

skr::io::IRAMService* SProject::GetRamService() const
{
    return ram_service;
}

bool SProject::OpenProject(const skr::String& project_name, const skr::String& root, const SProjectConfig& cfg) noexcept
{
    std::error_code ec = {};
    auto resolvePath = [&](const skr::String& path) -> skr::String {
        skr::String result;
        result.reserve(path.size() + 256); // Reserve extra space for expansions

        auto view = path.view();
        auto currentView = view;

        while (!currentView.is_empty())
        {
            // Find next variable marker
            auto varStart = currentView.find(u8"${");
            if (!varStart)
            {
                // No more variables, append the rest
                result.append(currentView);
                break;
            }

            // Append text before variable
            result.append(currentView.subview(0, varStart.index()));

            // Find variable end
            auto remainingView = currentView.subview(varStart.index() + 2);
            auto varEnd = remainingView.find(u8"}");
            if (!varEnd)
            {
                // Unclosed variable, treat as literal text
                SKR_LOG_WARN(u8"Unclosed variable in path: %s", path.c_str());
                result.append(currentView.subview(varStart.index()));
                break;
            }

            // Extract and resolve variable name
            auto varName = remainingView.subview(0, varEnd.index());
            if (varName == u8"platform")
            {
#if SKR_PLAT_WINDOWS
                result.append(u8"windows");
#elif SKR_PLAT_MACOSX
                result.append(u8"macosx");
#endif
            }
            else
            {
                // Unknown variable, keep as-is
                SKR_LOG_WARN(u8"Unknown variable '%s' in path: %s",
                    skr::String(varName).c_str(),
                    path.c_str());
                result.append(currentView.subview(varStart.index(), varEnd.index() + 3));
            }

            // Move to next part
            currentView = currentView.subview(varStart.index() + varEnd.index() + 3);
        }

        return result;
    };

    auto toAbsolutePath = [&](const skr::String& path) {
        auto resolved = resolvePath(path);
        skr::filesystem::path result{ resolved.c_str() };
        if (result.is_relative())
        {
            result = skr::filesystem::path(root.c_str()) / result;
        }
        return result.lexically_normal();
    };

    name = project_name;

    assetDirectory = toAbsolutePath(cfg.assetDirectory);
    resourceDirectory = toAbsolutePath(cfg.resourceDirectory);
    artifactsDirectory = toAbsolutePath(cfg.artifactsDirectory);
    dependencyDirectory = artifactsDirectory / "deps";

    // create resource VFS
    skr_vfs_desc_t resource_vfs_desc = {};
    resource_vfs_desc.app_name = name.u8_str();
    resource_vfs_desc.mount_type = SKR_MOUNT_TYPE_CONTENT;
    auto u8outputPath = resourceDirectory.u8string();
    resource_vfs_desc.override_mount_dir = u8outputPath.c_str();
    resource_vfs = skr_create_vfs(&resource_vfs_desc);

    // create asset VFS
    skr_vfs_desc_t asset_vfs_desc = {};
    asset_vfs_desc.app_name = name.u8_str();
    asset_vfs_desc.mount_type = SKR_MOUNT_TYPE_CONTENT;
    auto u8assetPath = assetDirectory.u8string();
    asset_vfs_desc.override_mount_dir = u8assetPath.c_str();
    asset_vfs = skr_create_vfs(&asset_vfs_desc);

    // create dependency VFS
    skr_vfs_desc_t dependency_vfs_desc = {};
    dependency_vfs_desc.app_name = name.u8_str();
    dependency_vfs_desc.mount_type = SKR_MOUNT_TYPE_CONTENT;
    auto u8dependencyPath = dependencyDirectory.u8string();
    dependency_vfs_desc.override_mount_dir = u8dependencyPath.c_str();
    dependency_vfs = skr_create_vfs(&dependency_vfs_desc);

    auto ioServiceDesc = make_zeroed<skr_ram_io_service_desc_t>();
    ioServiceDesc.name = u8"CompilerRAMIOService";
    ioServiceDesc.sleep_time = 1000 / 60;
    ram_service = skr_io_ram_service_t::create(&ioServiceDesc);
    ram_service->run();

    // Create output directories
    skr_vfs_mkdir(resource_vfs, u8".");
    skr_vfs_mkdir(dependency_vfs, u8".");
    return true;
}

bool SProject::OpenProject(const skr::filesystem::path& projectFilePath) noexcept
{
    auto projectPath = projectFilePath.lexically_normal();
    skd::SProjectConfig cfg;
    {
        // Create a temporary VFS for reading the project file
        skr_vfs_desc_t temp_vfs_desc = {};
        temp_vfs_desc.app_name = u8"TempProjectLoader";
        temp_vfs_desc.mount_type = SKR_MOUNT_TYPE_ABSOLUTE;
        auto temp_vfs = skr_create_vfs(&temp_vfs_desc);
        
        auto projectFile = skr_vfs_fopen(temp_vfs, (const char8_t*)projectPath.u8string().c_str(), 
                                         SKR_FM_READ_BINARY, SKR_FILE_CREATION_OPEN_EXISTING);
        if (!projectFile)
        {
            SKR_LOG_ERROR(u8"Failed to open project file: %s", projectPath.u8string().c_str());
            skr_free_vfs(temp_vfs);
            return false;
        }
        
        // read string from file
        auto fileSize = skr_vfs_fsize(projectFile);
        skr::String projectFileContent;
        projectFileContent.add(u8'0', fileSize);
        skr_vfs_fread(projectFile, projectFileContent.data_raw_w(), 0, fileSize);
        skr_vfs_fclose(projectFile);
        skr_free_vfs(temp_vfs);

        skr::archive::JsonReader reader(projectFileContent.view());
        if (!skr::json_read(&reader, cfg))
        {
            SKR_LOG_ERROR(u8"Failed to parse project file: %s", projectPath.c_str());
            return false;
        }
    }
    auto root = projectFilePath.parent_path().u8string();
    auto name = projectFilePath.filename().u8string();
    return OpenProject(name.c_str(), root.c_str(), cfg);
}

bool SProject::CloseProject() noexcept
{   
    return true;
}

bool SProject::LoadAssetData(skr::StringView uri, skr::Vector<uint8_t>& content) noexcept
{
    skr::String path = uri;
    auto asset_file = skr_vfs_fopen(asset_vfs, path.u8_str(), SKR_FM_READ_BINARY, SKR_FILE_CREATION_OPEN_EXISTING);
    const auto asset_size = skr_vfs_fsize(asset_file);
    content.resize_unsafe(asset_size);
    skr_vfs_fread(asset_file, content.data(), 0, asset_size);
    skr_vfs_fclose(asset_file);
    return true;
}

bool SProject::LoadAssetMeta(skr::StringView uri, skr::String& content) noexcept
{
    skr::String path = uri;
    auto asset_file = skr_vfs_fopen(asset_vfs, path.u8_str(), SKR_FM_READ_BINARY, SKR_FILE_CREATION_OPEN_EXISTING);
    const auto asset_size = skr_vfs_fsize(asset_file);
    content.add(u8'0', asset_size);
    skr_vfs_fread(asset_file, content.data_raw_w(), 0, asset_size);
    skr_vfs_fclose(asset_file);
    return true;
}

SProject::~SProject() noexcept
{
    if (ram_service)
        skr_io_ram_service_t::destroy(ram_service);
    if (dependency_vfs)
        skr_free_vfs(dependency_vfs);
    if (resource_vfs)
        skr_free_vfs(resource_vfs);
    if (asset_vfs)
        skr_free_vfs(asset_vfs);
}
} // namespace skd