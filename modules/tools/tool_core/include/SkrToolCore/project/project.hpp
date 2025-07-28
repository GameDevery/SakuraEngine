#pragma once
#include "SkrToolCore/fwd_types.hpp"
#include "SkrOS/filesystem.hpp"
#include "SkrContainers/string.hpp"
#ifndef __meta__
    #include "SkrToolCore/project/project.generated.h" // IWYU pragma: export
#endif

namespace skd
{

sreflect_struct(
    guid = "D153957A-2272-45F8-92DA-EEEB67821D20" serde = @json)
SProjectConfig
{
    skr::String assetDirectory;
    skr::String resourceDirectory;
    skr::String artifactsDirectory;
};

struct TOOL_CORE_API SProject
{
public:
    ~SProject() noexcept;

    skr::filesystem::path GetAssetPath() const noexcept { return assetDirectory; }
    skr::filesystem::path GetOutputPath() const noexcept { return resourceDirectory; }
    skr::filesystem::path GetDependencyPath() const noexcept { return dependencyDirectory; }
    bool LoadAssetData(skr::StringView uri, skr::Vector<uint8_t>& content) noexcept;
    bool LoadAssetMeta(skr::StringView uri, skr::String& content) noexcept;
    
    // void SetAssetVFS(skr_vfs_t* asset_vfs);
    // void SetResourceVFS(skr_vfs_t* resource_vfs);
    // void SetRAMService(skr::io::IRAMService* service);
    skr_vfs_t* GetAssetVFS() const;
    skr_vfs_t* GetResourceVFS() const;
    skr::io::IRAMService* GetRamService() const;

    static SProject* OpenProject(const skr::filesystem::path& path) noexcept;
    static SProject* OpenProject(const skr::String& name, const skr::String& root, const SProjectConfig& config) noexcept;
    static void CloseProject(SProject* project) noexcept;
    static void SetWorkspace(const skr::filesystem::path& path) noexcept;

private:
    skr_vfs_t* asset_vfs = nullptr;
    skr_vfs_t* resource_vfs = nullptr;
    skr::io::IRAMService* ram_service = nullptr;

    skr::filesystem::path assetDirectory;
    skr::filesystem::path artifactsDirectory;
    skr::filesystem::path resourceDirectory;
    skr::filesystem::path dependencyDirectory;
    skr::String name;
};
} // namespace skd