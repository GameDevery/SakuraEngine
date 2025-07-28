#include "SkrCore/log.h"
#include "SkrBase/misc/make_zeroed.hpp"
#include "SkrCore/platform/vfs.h"
#include "SkrRT/io/ram_io.hpp"
#include "SkrToolCore/project/project.hpp"
#include "SkrSerde/json_serde.hpp"

namespace skd
{
static skr::filesystem::path Workspace;
void SProject::SetWorkspace(const skr::filesystem::path& path) noexcept
{
    Workspace = path;
}

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

skr::io::IRAMService* SProject::GetRamService() const
{
    return ram_service;
}

SProject* SProject::OpenProject(const skr::String& name, const skr::String& root, const SProjectConfig& cfg) noexcept
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
            if (varName == u8"workspace")
            {
                result.append(Workspace.u8string().c_str());
            }
            else if (varName == u8"platform")
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

    auto project = SkrNew<skd::SProject>();
    project->name = name;

    project->assetDirectory = toAbsolutePath(cfg.assetDirectory);
    project->resourceDirectory = toAbsolutePath(cfg.resourceDirectory);
    project->artifactsDirectory = toAbsolutePath(cfg.artifactsDirectory);
    project->dependencyDirectory = project->artifactsDirectory / "deps";

    // create resource VFS
    skr_vfs_desc_t resource_vfs_desc = {};
    resource_vfs_desc.app_name = project->name.u8_str();
    resource_vfs_desc.mount_type = SKR_MOUNT_TYPE_CONTENT;
    auto u8outputPath = project->resourceDirectory.u8string();
    resource_vfs_desc.override_mount_dir = u8outputPath.c_str();
    project->resource_vfs = skr_create_vfs(&resource_vfs_desc);

    // create asset VFS
    skr_vfs_desc_t asset_vfs_desc = {};
    asset_vfs_desc.app_name = project->name.u8_str();
    asset_vfs_desc.mount_type = SKR_MOUNT_TYPE_CONTENT;
    auto u8assetPath = project->assetDirectory.u8string();
    asset_vfs_desc.override_mount_dir = u8assetPath.c_str();
    project->asset_vfs = skr_create_vfs(&asset_vfs_desc);

    auto ioServiceDesc = make_zeroed<skr_ram_io_service_desc_t>();
    ioServiceDesc.name = u8"CompilerRAMIOService";
    ioServiceDesc.sleep_time = 1000 / 60;
    project->ram_service = skr_io_ram_service_t::create(&ioServiceDesc);
    project->ram_service->run();

    // Create output dir
    skr::filesystem::create_directories(project->GetOutputPath(), ec);
    return project;
}

SProject* SProject::OpenProject(const skr::filesystem::path& projectFilePath) noexcept
{
    auto projectPath = projectFilePath.lexically_normal().string();
    skd::SProjectConfig cfg;
    {
        auto projectFile = fopen(projectPath.c_str(), "rb");
        if (!projectFile)
        {
            SKR_LOG_ERROR(u8"Failed to open project file: %s", projectPath.c_str());
            return nullptr;
        }
        // read string from file with c <file>
        fseek(projectFile, 0, SEEK_END);
        auto fileSize = ftell(projectFile);
        fseek(projectFile, 0, SEEK_SET);
        skr::String projectFileContent;
        projectFileContent.add(u8'0', fileSize);
        fread(projectFileContent.data_raw_w(), 1, fileSize, projectFile);
        fclose(projectFile);

        skr::archive::JsonReader reader(projectFileContent.view());
        if (!skr::json_read(&reader, cfg))
        {
            SKR_LOG_ERROR(u8"Failed to parse project file: %s", projectPath.c_str());
            return nullptr;
        }
    }
    auto root = projectFilePath.parent_path().u8string();
    auto name = projectFilePath.filename().u8string();
    return OpenProject(name.c_str(), root.c_str(), cfg);
}

void SProject::CloseProject(SProject *project) noexcept
{
    SkrDelete(project);
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
    if (ram_service) skr_io_ram_service_t::destroy(ram_service);
    if (resource_vfs) skr_free_vfs(resource_vfs);
    if (asset_vfs) skr_free_vfs(asset_vfs);
}
} // namespace skd