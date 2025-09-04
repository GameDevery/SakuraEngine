#pragma once
#include "AnimSampleCore/config.h"
#include "SkrToolCore/project/project.hpp"
#include "SkrOS/filesystem.hpp"

namespace skr
{
struct ANIM_SAMPLE_CORE_API AnimSampleProject : public skd::SProject
{
    void Initialize(skr::String _name, skr::String _path) SKR_NOEXCEPT;
    bool LoadAssetMeta(const skd::URI& uri, skr::String& content) noexcept override;
    bool SaveAssetMeta(const skd::URI& uri, const skr::String& content) noexcept override;
    bool ExistImportedAsset(const skd::URI& uri);
    void SaveToDisk();
    void LoadFromDisk();
    skr::String proj_name;
    skr::String proj_path;
    skr::Path proj_f_path;
    skr::ParallelFlatHashMap<skd::URI, skr::String, skr::Hash<skd::URI>> MetaDatabase;
};
} // namespace skr