#include "AnimSampleCore/project.h"
#include "SkrContainers/hashmap.hpp"

namespace skr
{

void AnimSampleProject::Initialize(skr::String _name, skr::String _path) SKR_NOEXCEPT
{
    proj_name = _name;
    proj_path = _path;
    // create path if not exists
    auto proj_f_name = proj_name;
    proj_f_name.append(u8".sproject");
    proj_f_path = skr::Path(proj_path) / proj_f_name;
}

bool AnimSampleProject::LoadAssetMeta(const skd::URI& uri, skr::String& content) noexcept
{
    if (MetaDatabase.contains(uri))
    {
        content = MetaDatabase[uri];
        return true;
    }
    return false;
}

bool AnimSampleProject::SaveAssetMeta(const skd::URI& uri, const skr::String& content) noexcept
{
    MetaDatabase[uri] = content;
    return true;
}

bool AnimSampleProject::ExistImportedAsset(const skd::URI& uri)
{
    return MetaDatabase.contains(uri);
}

void AnimSampleProject::SaveToDisk()
{
    skr::archive::JsonWriter writer(4);
    writer.StartObject();
    writer.Key(u8"assets");
    skr::json_write(&writer, MetaDatabase);
    writer.EndObject();

    auto json_str = writer.Write();

    if (skr::fs::File::write_all_text(proj_f_path, json_str.view()))
        SKR_LOG_INFO(u8"[AnimSampleProject] Project saved to: %s", proj_f_path.c_str());
    else
        SKR_LOG_ERROR(u8"[AnimSampleProject] Failed to save project to: %s", proj_f_path.c_str());
}

void AnimSampleProject::LoadFromDisk()
{

    skr::String json_content;
    if (skr::fs::File::read_all_text(proj_f_path, json_content))
    {
        // Parse JSON
        skr::archive::JsonReader reader(json_content.view());
        reader.StartObject();
        reader.Key(u8"assets");
        {
            skr::json_read(&reader, MetaDatabase);
            SKR_LOG_INFO(u8"[AnimSampleProject] Project loaded from: %s, %zu assets",
                proj_f_path.c_str(),
                MetaDatabase.size());
        }
        reader.EndObject();
    }
    else
    {
        // File doesn't exist, which is fine - just means empty project
        MetaDatabase.clear();
        SKR_LOG_INFO(u8"[AnimSampleProject] No existing project file found at: %s, starting with empty project", proj_f_path.c_str());
    }
}

} // namespace skr