#include <SkrV8/v8_vfs.hpp>
#include <SkrOS/filesystem.hpp>

namespace skr
{
// path operations
String V8VFSSystemFS::path_normalize(String raw_path)
{
    Path path{ raw_path };
    return path.normalize().to_string();
}
String V8VFSSystemFS::path_solve_relative_module(String referrer, String relative_path)
{
    Path path_referrer{ referrer };
    Path path_relative{ relative_path };
    return (path_referrer.parent_directory() / path_relative).normalize().to_string();
}

// io operations
Optional<String> V8VFSSystemFS::load_script(String path)
{
    Path full_path = Path{ root_path } / path;

    // check exist
    if (!fs::File::exists(full_path))
    {
        return {};
    }

    // read file content
    Vector<uint8_t> file_data;
    if (!fs::File::read_all_bytes(full_path, file_data))
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