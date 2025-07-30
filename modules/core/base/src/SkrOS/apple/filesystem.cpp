#include "SkrOS/filesystem.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <cstring>

namespace skr::fs
{

#ifdef __APPLE__

// Apple 特定的 Directory 方法实现
Path Directory::application_support()
{
    auto home_path = home();
    return home_path / u8"Library" / u8"Application Support";
}

Path Directory::caches()
{
    auto home_path = home();
    return home_path / u8"Library" / u8"Caches";
}

Path Directory::bundle_path()
{
    CFBundleRef bundle = CFBundleGetMainBundle();
    if (!bundle)
        return Path();
    
    CFURLRef url = CFBundleCopyBundleURL(bundle);
    if (!url)
        return Path();
    
    char path[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(url, TRUE, reinterpret_cast<UInt8*>(path), PATH_MAX))
    {
        CFRelease(url);
        return Path();
    }
    
    CFRelease(url);
    return Path(skr::String(reinterpret_cast<const char8_t*>(path)));
}

// Apple 特定的 File 方法实现
bool File::is_bundle(const Path& path)
{
    auto info = get_info(path);
    if (!info.is_directory())
        return false;
    
    const auto& path_str = path.string();
    return path_str.contains(u8".app") || 
           path_str.contains(u8".framework") ||
           path_str.contains(u8".bundle");
}

Path File::get_bundle_resource_path(const Path& bundle_path)
{
    return bundle_path / u8"Contents" / u8"Resources";
}

#endif // __APPLE__

} // namespace skr::fs