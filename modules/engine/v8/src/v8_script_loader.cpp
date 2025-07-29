#include <SkrV8/v8_script_loader.hpp>
#include <SkrOS/filesystem.hpp>

namespace skr
{
String V8ScriptLoaderSystemFS::normalize_path(String raw_path)
{
    raw_path.replace(u8"\\", u8"/");
    return raw_path;
}
Optional<String> V8ScriptLoaderSystemFS::load_script(String path)
{
    const skr::Path fs_path{path};

    // check exists
    if (!skr::fs::File::exists(fs_path))
    {
        SKR_LOG_ERROR(u8"script file not found: %s", path.c_str());
        return {};
    }

    // read file content
    skr::Vector<uint8_t> file_data;
    if (!skr::fs::File::read_all_bytes(fs_path, file_data))
    {
        SKR_LOG_ERROR(u8"failed to read script file: %s", path.c_str());
        return {};
    }

    // convert to string
    String content;
    content.resize_unsafe(file_data.size());
    memcpy(content.data_w(), file_data.data(), file_data.size());
    
    return content;
}
} // namespace skr