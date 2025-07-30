#include "SkrBase/containers/path/path.hpp"
#include "SkrContainersDef/vector.hpp"

namespace skr
{

// constructors
Path::Path(uint8_t style)
    : _style(style)
{
}

Path::Path(const skr::String& str, uint8_t style)
    : _path(str), _style(style)
{
    normalize_separators();
}

Path::Path(skr::String&& str, uint8_t style)
    : _path(std::move(str)), _style(style)
{
    normalize_separators();
}

Path::Path(const char8_t* str, uint8_t style)
    : _path(str), _style(style)
{
    normalize_separators();
}

Path::Path(skr::StringView str, uint8_t style)
    : _path(str), _style(style)
{
    normalize_separators();
}

// path operations
Path Path::parent_directory() const
{
    if (_path.is_empty())
        return Path(_style);
    
    // Root paths return themselves
    if (is_root())
        return *this;
    
    // Remove trailing slashes first (except for root paths)
    skr::StringView path_view = _path;
    while (path_view.size() > 1 && path_view.ends_with(u8"/"))
    {
        path_view = path_view.subview(0, path_view.size() - 1);
    }
    
    // Find last separator
    auto last_sep = path_view.find_last(u8"/");
    if (!last_sep)
        return Path(_style);
    
    // Handle root paths
    if (last_sep.index() == 0) // Unix root "/"
        return Path(u8"/", _style);
    
    // Windows drive root like "C:/"
    if ((_style & Style::Windows) && last_sep.index() == 2 && path_view.at_buffer(1) == ':')
        return Path(path_view.subview(0, 3), _style);
    
    return Path(path_view.subview(0, last_sep.index()), _style);
}

Path Path::filename() const
{
    if (_path.is_empty())
        return Path(_style);
    
    auto last_sep = _path.find_last(u8"/");
    if (!last_sep)
        return *this;
    
    return Path(_path.subview(last_sep.index() + 1), _style);
}

Path Path::basename() const
{
    auto fname = filename();
    if (fname._path.is_empty())
        return Path(_style);
    
    // Special cases for "." and ".."
    if (fname._path == u8"." || fname._path == u8"..")
        return fname;
    
    auto last_dot = fname._path.find_last(u8".");
    if (!last_dot || last_dot.index() == 0)
        return fname;
    
    return Path(fname._path.subview(0, last_dot.index()), _style);
}

Path Path::extension(bool with_dot) const
{
    auto fname = filename();
    if (fname._path.is_empty())
        return Path(_style);
    
    // Special cases for "." and ".."
    if (fname._path == u8"." || fname._path == u8"..")
        return Path(_style);
    
    auto last_dot = fname._path.find_last(u8".");
    if (!last_dot || last_dot.index() == 0)
        return Path(_style);
    
    if (with_dot)
        return Path(fname._path.subview(last_dot.index()), _style);
    else
        return Path(fname._path.subview(last_dot.index() + 1), _style);
}

Path& Path::set_filename(const Path& new_filename)
{
    // If path ends with separator, just append the filename
    if (!_path.is_empty() && _path.ends_with(u8"/"))
    {
        _path.append(new_filename._path);
        return *this;
    }
    // Otherwise, replace the current filename
    else
    {
        auto parent = parent_directory();
        _path = std::move(parent._path);
        return append(new_filename);
    }
}

Path& Path::join(const Path& p)
{
    return append(p);
}

Path& Path::set_extension(const Path& replacement)
{
    auto fname = filename();
    if (fname._path.is_empty())
        return *this;
    
    auto last_dot = _path.find_last(u8".");
    auto last_slash = _path.find_last(u8"/");
    if (last_dot && (!last_slash || last_dot.index() > last_slash.index()))
    {
        _path.replace_range(replacement._path, last_dot.index());
    }
    else
    {
        _path.append(replacement._path);
    }
    
    return *this;
}

Path& Path::remove_extension()
{
    auto fname = filename();
    if (fname._path.is_empty())
        return *this;
    
    // Special cases for "." and ".."
    if (fname._path == u8"." || fname._path == u8"..")
        return *this;
    
    auto last_dot = _path.find_last(u8".");
    auto last_slash = _path.find_last(u8"/");
    
    // Make sure the dot is in the filename part, not in a directory name
    if (last_dot && (!last_slash || last_dot.index() > last_slash.index()))
    {
        // Also check it's not a hidden file (starting with dot)
        bool is_hidden = false;
        if (last_slash)
        {
            is_hidden = (last_dot.index() == last_slash.index() + 1);
        }
        else
        {
            is_hidden = (last_dot.index() == 0);
        }
        
        if (!is_hidden)
        {
            _path.resize_unsafe(last_dot.index());
        }
    }
    
    return *this;
}

Path& Path::replace_extension(const Path& replacement)
{
    remove_extension();
    if (!replacement._path.is_empty())
    {
        _path.append(replacement._path);
    }
    return *this;
}

Path& Path::append(const Path& p)
{
    if (_path.is_empty())
    {
        _path = p._path;
        return *this;
    }
    
    // Check if p is absolute
    if (is_absolute_path(p._path, p._style))
    {
        _path = p._path;
        return *this;
    }
    
    // Add separator if needed
    if (!_path.ends_with(u8"/"))
        _path.append(u8"/");
    
    // Even if p is empty, we've already added the separator
    if (!p._path.is_empty())
        _path.append(p._path);
    
    return *this;
}

Path& Path::concat(const Path& p)
{
    _path.append(p._path);
    return *this;
}

Path Path::normalize() const
{
    if (_path.is_empty())
        return Path(_style);
    
    skr::String result;
    skr::String root_part;
    
    // Extract root part if exists
    size_t path_start = 0;
    if (_path.starts_with(u8"/"))
    {
        root_part = u8"/";
        path_start = 1;
    }
    else if ((_style & Style::Windows) && _path.size() >= 3 && 
             _path.at_buffer(1) == ':' && _path.at_buffer(2) == '/')
    {
        char8_t drive = _path.at_buffer(0);
        if ((drive >= 'A' && drive <= 'Z') || (drive >= 'a' && drive <= 'z'))
        {
            // Normalize drive letter to uppercase
            char8_t normalized_drive[4] = { 0, ':', '/', 0 };
            normalized_drive[0] = (drive >= 'a' && drive <= 'z') ? (drive - 'a' + 'A') : drive;
            root_part = skr::String(reinterpret_cast<const char8_t*>(normalized_drive));
            path_start = 3;
        }
    }
    
    // Process path components
    skr::StringView remaining = _path.subview(path_start);
    
    // Two-pass approach: first count components to handle ".."
    struct Component {
        size_t start;
        size_t length;
        bool is_dotdot;
    };
    
    // Parse components
    Component components[256]; // Max components
    size_t component_count = 0;
    
    remaining.split_each([&](const skr::StringView& part) {
        if (!part.is_empty() && part != u8".")
        {
            if (component_count < 256)
            {
                components[component_count].start = part.data() - remaining.data();
                components[component_count].length = part.size();
                components[component_count].is_dotdot = (part == u8"..");
                component_count++;
            }
        }
        return true;
    }, u8"/", true);
    
    // Process ".." components
    bool keep[256] = {false};
    int output_count = 0;
    
    for (size_t i = 0; i < component_count; ++i)
    {
        if (components[i].is_dotdot)
        {
            // Try to cancel out with previous component
            int j = static_cast<int>(i) - 1;
            while (j >= 0 && (!keep[j] || components[j].is_dotdot))
                j--;
            
            if (j >= 0)
            {
                // Found a component to cancel
                keep[j] = false;
                output_count--;
            }
            else if (root_part.is_empty())
            {
                // Relative path, keep the ".."
                keep[i] = true;
                output_count++;
            }
            // else: absolute path, ignore ".." at root
        }
        else
        {
            keep[i] = true;
            output_count++;
        }
    }
    
    // Build result
    result = root_part;
    
    // Add kept components
    bool first = true;
    for (size_t i = 0; i < component_count; ++i)
    {
        if (keep[i])
        {
            if (!first || !root_part.is_empty())
                result.append(u8"/");
            result.append(remaining.subview(components[i].start, components[i].length));
            first = false;
        }
    }
    
    // Handle empty result
    if (result.is_empty())
        result = u8".";
    
    return Path(std::move(result), _style);
}

Path Path::relative_to(const Path& base) const
{
    // Both paths must be absolute or both relative
    if (is_absolute() != base.is_absolute())
        return Path(_style);
    
    auto norm_this = normalize();
    auto norm_base = base.normalize();
    
    // Special case: identical paths
    if (norm_this._path == norm_base._path)
        return Path(u8".", _style);
    
    // Split paths into components
    struct PathComponents {
        skr::StringView components[256];
        size_t count = 0;
        
        void parse(const skr::String& path) {
            count = 0;
            path.split_each([this](const skr::StringView& part) {
                if (!part.is_empty() && count < 256) {
                    components[count++] = part;
                }
                return true;
            }, u8"/", true);
        }
    };
    
    PathComponents this_components, base_components;
    this_components.parse(norm_this._path);
    base_components.parse(norm_base._path);
    
    // Find common prefix
    size_t common = 0;
    while (common < this_components.count && 
           common < base_components.count &&
           this_components.components[common] == base_components.components[common])
    {
        common++;
    }
    
    // Build relative path
    skr::String result;
    
    // Add ".." for each remaining base component
    for (size_t i = common; i < base_components.count; ++i)
    {
        if (!result.is_empty())
            result.append(u8"/");
        result.append(u8"..");
    }
    
    // Add remaining this components
    for (size_t i = common; i < this_components.count; ++i)
    {
        if (!result.is_empty())
            result.append(u8"/");
        result.append(this_components.components[i]);
    }
    
    if (result.is_empty())
        return Path(u8".", _style);
    
    return Path(std::move(result), _style);
}

// query operations
bool Path::is_empty() const
{
    return _path.is_empty();
}

bool Path::is_absolute() const
{
    return is_absolute_path(_path, _style);
}

bool Path::is_relative() const
{
    return !is_absolute();
}

bool Path::is_root() const
{
    if (_path.is_empty())
        return false;
    
    // Unix root
    if ((_style & Style::Unix) && _path == u8"/")
        return true;
    
    // Windows root like "C:/" or "C:"
    if ((_style & Style::Windows) && _path.size() >= 2 && _path.at_buffer(1) == ':')
    {
        char8_t first = _path.at_buffer(0);
        if ((first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z'))
        {
            if (_path.size() == 2) // "C:"
                return true;
            if (_path.size() == 3 && _path.at_buffer(2) == '/') // "C:/"
                return true;
        }
    }
    
    return false;
}


bool Path::has_parent() const
{
    if (_path.is_empty() || is_root())
        return false;
    return _path.contains(u8"/");
}

bool Path::has_filename() const
{
    if (_path.is_empty())
        return false;
    
    // Check if path ends with separator
    if (_path.ends_with(u8"/"))
        return false;
    
    auto last_sep = _path.find_last(u8"/");
    if (last_sep)
        return last_sep.index() < _path.size() - 1;
    
    return true;
}


bool Path::has_extension() const
{
    auto fname = filename();
    if (fname._path.is_empty())
        return false;
    
    // Special cases for "." and ".."
    if (fname._path == u8"." || fname._path == u8"..")
        return false;
    
    auto last_dot = fname._path.find_last(u8".");
    return last_dot && last_dot.index() > 0 && last_dot.index() < fname._path.size() - 1;
}

// conversions
const skr::String& Path::string() const
{
    return _path;
}

skr::String Path::native_string() const
{
#ifdef _WIN32
    if (_style & Style::Windows)
    {
        // Convert to native Windows format if needed
        skr::String result = _path;
        result.replace(u8"/", u8"\\");
        return result;
    }
#endif
    return _path;
}

// operators
Path& Path::operator/=(const Path& p)
{
    return append(p);
}

Path& Path::operator+=(const Path& p)
{
    return concat(p);
}

Path Path::operator/(const Path& p) const
{
    Path result = *this;
    result /= p;
    return result;
}

Path Path::operator+(const Path& p) const
{
    Path result = *this;
    result += p;
    return result;
}

bool Path::operator==(const Path& p) const
{
    return _path == p._path;
}

bool Path::operator!=(const Path& p) const
{
    return _path != p._path;
}

bool Path::operator<(const Path& p) const
{
    return _path < p._path;
}

bool Path::operator<=(const Path& p) const
{
    return _path <= p._path;
}

bool Path::operator>(const Path& p) const
{
    return _path > p._path;
}

bool Path::operator>=(const Path& p) const
{
    return _path >= p._path;
}

// private helpers
void Path::normalize_separators()
{
    // Replace all backslashes with forward slashes
    _path.replace(u8"\\", u8"/");
    
    // Remove duplicate slashes (except for UNC paths)
    while (_path.contains(u8"//"))
    {
        _path.replace(u8"//", u8"/");
    }
}

bool Path::is_absolute_path(skr::StringView path, uint8_t style)
{
    if (path.is_empty())
        return false;
    
    // Unix absolute path
    if ((style & Style::Unix) && path.starts_with(u8"/"))
        return true;
    
    // Windows absolute path
    if (style & Style::Windows)
    {
        if (path.size() >= 3 && path.at_buffer(1) == ':' && path.at_buffer(2) == '/')
        {
            char8_t first = path.at_buffer(0);
            return (first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z');
        }
    }
    
    return false;
}

bool Path::is_root_component(skr::StringView component, uint8_t style)
{
    if (component == u8"/")
        return true;
    
    if (style & Style::Windows)
    {
        // Check for drive letter
        if (component.size() == 2 && component.at_buffer(1) == ':')
        {
            char8_t first = component.at_buffer(0);
            return (first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z');
        }
    }
    
    return false;
}

} // namespace skr