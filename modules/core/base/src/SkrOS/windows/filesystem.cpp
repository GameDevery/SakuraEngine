#include "SkrOS/filesystem.hpp"
#include "../winheaders.h"
#include <direct.h>
#include <io.h>
#include <sys/stat.h>

namespace skr::fs
{

// UTF-8 到 UTF-16 转换辅助函数
static skr::Vector<wchar_t> utf8_to_utf16(const skr::String& utf8_str)
{
    if (utf8_str.is_empty())
        return skr::Vector<wchar_t>();
    
    // 使用引擎的字符串转换功能
    auto wide_len = utf8_str.to_wide_length();
    skr::Vector<wchar_t> result;
    result.resize_zeroed(wide_len + 1); // +1 for null terminator
    utf8_str.to_wide(result.data());
    result[wide_len] = 0; // Null terminate
    
    return result;
}

// UTF-16 到 UTF-8 转换辅助函数
static skr::String utf16_to_utf8(const wchar_t* wide_str)
{
    return skr::String::FromWide(wide_str);
}

// 声明在 build.filesystem.cpp 中定义的函数
extern Error get_last_error();
extern void set_last_error(Error error);
extern Error g_last_error;

// 错误码转换
Error win32_error_to_filesystem_error(DWORD error)
{
    switch (error) {
        case ERROR_SUCCESS: return Error::None;
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND: return Error::NotFound;
        case ERROR_ACCESS_DENIED: return Error::AccessDenied;
        case ERROR_FILE_EXISTS:
        case ERROR_ALREADY_EXISTS: return Error::AlreadyExists;
        case ERROR_DIRECTORY: return Error::NotADirectory;
        case ERROR_DIR_NOT_EMPTY: return Error::DirectoryNotEmpty;
        case ERROR_INVALID_NAME:
        case ERROR_BAD_PATHNAME: return Error::InvalidPath;
        case ERROR_TOO_MANY_OPEN_FILES: return Error::TooManyOpenFiles;
        case ERROR_DISK_FULL: return Error::DiskFull;
        case ERROR_WRITE_PROTECT: return Error::ReadOnlyFileSystem;
        default: return Error::Unknown;
    }
}

FileType win32_attributes_to_filetype(DWORD attributes)
{
    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
        return FileType::Directory;
    if (attributes & FILE_ATTRIBUTE_REPARSE_POINT)
        return FileType::Symlink;
    return FileType::Regular;
}

// ============================================================================
// File 实现
// ============================================================================

bool File::exists(const Path& path)
{
    set_last_error(Error::None);
    
    auto wide_path = utf8_to_utf16(path.string());
    if (wide_path.is_empty() && !path.is_empty())
    {
        set_last_error(Error::InvalidPath);
        return false;
    }
    
    DWORD attributes = GetFileAttributesW(wide_path.data());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        set_last_error(win32_error_to_filesystem_error(GetLastError()));
        return false;
    }
    return !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

bool File::is_regular_file(const Path& path)
{
    return exists(path);
}

FileInfo File::get_info(const Path& path)
{
    FileInfo info;
    info.path = path;
    set_last_error(Error::None);
    
    auto wide_path = utf8_to_utf16(path.string());
    if (wide_path.is_empty() && !path.is_empty())
    {
        set_last_error(Error::InvalidPath);
        return info;
    }
    
    WIN32_FILE_ATTRIBUTE_DATA fileData;
    if (!GetFileAttributesExW(wide_path.data(), 
                             GetFileExInfoStandard, &fileData))
    {
        set_last_error(win32_error_to_filesystem_error(GetLastError()));
        return info;
    }
    
    info.type = win32_attributes_to_filetype(fileData.dwFileAttributes);
    info.size = (static_cast<uint64_t>(fileData.nFileSizeHigh) << 32) | fileData.nFileSizeLow;
    
    // 转换时间戳
    auto convert_filetime = [](const FILETIME& ft) -> FileTime {
        return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    };
    
    info.creation_time = convert_filetime(fileData.ftCreationTime);
    info.last_write_time = convert_filetime(fileData.ftLastWriteTime);
    info.last_access_time = convert_filetime(fileData.ftLastAccessTime);
    
    info.platform_attrs.windows_attributes = fileData.dwFileAttributes;
    
    return info;
}

Error File::get_last_error()
{
    return g_last_error;
}

bool File::remove(const Path& path)
{
    set_last_error(Error::None);
    
    auto wide_path = utf8_to_utf16(path.string());
    if (wide_path.is_empty() && !path.is_empty())
    {
        set_last_error(Error::InvalidPath);
        return false;
    }
    
    if (!DeleteFileW(wide_path.data()))
    {
        set_last_error(win32_error_to_filesystem_error(GetLastError()));
        return false;
    }
    return true;
}

bool File::copy(const Path& from, const Path& to, CopyOptions options)
{
    set_last_error(Error::None);
    
    // Check if we should skip existing files
    bool skip_existing = (static_cast<uint32_t>(options) & static_cast<uint32_t>(CopyOptions::SkipExisting)) != 0;
    bool overwrite_existing = (static_cast<uint32_t>(options) & static_cast<uint32_t>(CopyOptions::OverwriteExisting)) != 0;
    
    // If skip_existing is set and destination exists, return success without copying
    if (skip_existing && exists(to))
    {
        return true;
    }
    
    auto wide_from = utf8_to_utf16(from.string());
    auto wide_to = utf8_to_utf16(to.string());
    if ((wide_from.is_empty() && !from.is_empty()) || (wide_to.is_empty() && !to.is_empty()))
    {
        set_last_error(Error::InvalidPath);
        return false;
    }
    
    // fail_if_exists should be true only if neither SkipExisting nor OverwriteExisting is set
    BOOL fail_if_exists = !overwrite_existing && !skip_existing;
    
    if (!CopyFileW(wide_from.data(), wide_to.data(), fail_if_exists))
    {
        set_last_error(win32_error_to_filesystem_error(GetLastError()));
        return false;
    }
    
    return true;
}

uint64_t File::size(const Path& path)
{
    auto info = get_info(path);
    return info.exists() ? info.size : static_cast<uint64_t>(-1);
}

// File 实例方法
bool File::open(const Path& path, OpenMode mode)
{
    close();
    last_error_ = Error::None;
    
    DWORD access = 0;
    DWORD creation = 0;
    DWORD flags = 0;
    
    if (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OpenMode::Read)) access |= GENERIC_READ;
    if (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OpenMode::Write)) access |= GENERIC_WRITE;
    
    // Handle append mode - it should not truncate the file
    bool is_append = (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OpenMode::Append)) != 0;
    bool should_truncate = (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OpenMode::Truncate)) != 0 && !is_append;
    
    if (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OpenMode::Create))
    {
        if (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OpenMode::Exclusive))
            creation = CREATE_NEW;
        else if (should_truncate)
            creation = CREATE_ALWAYS;
        else
            creation = OPEN_ALWAYS;
    }
    else
    {
        if (should_truncate)
            creation = TRUNCATE_EXISTING;
        else
            creation = OPEN_EXISTING;
    }
    
    auto wide_path = utf8_to_utf16(path.string());
    if (wide_path.is_empty() && !path.is_empty())
    {
        last_error_ = Error::InvalidPath;
        return false;
    }
    
    handle_ = CreateFileW(
        wide_path.data(),
        access,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        creation,
        flags,
        nullptr
    );
    
    if (handle_ == INVALID_HANDLE_VALUE)
    {
        last_error_ = win32_error_to_filesystem_error(GetLastError());
        handle_ = nullptr;
        return false;
    }
    
    // If append mode is specified, seek to end of file
    if (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OpenMode::Append))
    {
        LARGE_INTEGER offset;
        offset.QuadPart = 0;
        if (!SetFilePointerEx(handle_, offset, nullptr, FILE_END))
        {
            last_error_ = win32_error_to_filesystem_error(GetLastError());
            CloseHandle(handle_);
            handle_ = nullptr;
            return false;
        }
    }
    
    return true;
}

void File::close()
{
    if (handle_)
    {
        CloseHandle(handle_);
        handle_ = nullptr;
        last_error_ = Error::None;
    }
}

bool File::is_open() const
{
    return handle_ != nullptr;
}

size_t File::read(void* buffer, size_t size)
{
    if (!handle_)
    {
        last_error_ = Error::NotFound;
        return 0;
    }
    
    DWORD bytes_read = 0;
    if (!ReadFile(handle_, buffer, static_cast<DWORD>(size), &bytes_read, nullptr))
    {
        last_error_ = win32_error_to_filesystem_error(GetLastError());
        return 0;
    }
    return bytes_read;
}

size_t File::write(const void* buffer, size_t size)
{
    if (!handle_)
    {
        last_error_ = Error::NotFound;
        return 0;
    }
    
    DWORD bytes_written = 0;
    if (!WriteFile(handle_, buffer, static_cast<DWORD>(size), &bytes_written, nullptr))
    {
        last_error_ = win32_error_to_filesystem_error(GetLastError());
        return 0;
    }
    return bytes_written;
}

int64_t File::get_size() const
{
    if (!handle_)
        return -1;
    
    LARGE_INTEGER size;
    if (!GetFileSizeEx(handle_, &size))
        return -1;
    return size.QuadPart;
}

// ============================================================================
// Directory 实现
// ============================================================================

bool Directory::exists(const Path& path)
{
    set_last_error(Error::None);
    
    auto wide_path = utf8_to_utf16(path.string());
    if (wide_path.is_empty() && !path.is_empty())
    {
        set_last_error(Error::InvalidPath);
        return false;
    }
    
    DWORD attributes = GetFileAttributesW(wide_path.data());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        set_last_error(win32_error_to_filesystem_error(GetLastError()));
        return false;
    }
    return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool Directory::is_directory(const Path& path)
{
    return exists(path);
}

bool Directory::create(const Path& path, bool recursive)
{
    set_last_error(Error::None);
    
    if (exists(path))
        return true;
    
    if (recursive)
    {
        auto parent = path.parent_directory();
        if (!parent.is_empty() && !exists(parent))
        {
            if (!create(parent, true))
                return false;
        }
    }
    
    auto wide_path = utf8_to_utf16(path.string());
    if (wide_path.is_empty() && !path.is_empty())
    {
        set_last_error(Error::InvalidPath);
        return false;
    }
    
    if (!CreateDirectoryW(wide_path.data(), nullptr))
    {
        set_last_error(win32_error_to_filesystem_error(GetLastError()));
        return false;
    }
    
    return true;
}

bool Directory::remove(const Path& path, bool recursive)
{
    set_last_error(Error::None);
    
    if (!exists(path))
        return false;
    
    if (recursive)
    {
        // Remove all contents recursively
        for (DirectoryIterator iter(path); !iter.at_end(); ++iter)
        {
            const auto& entry = *iter;
            if (entry.type == FileType::Directory)
            {
                if (!remove(entry.path, true))
                    return false;
            }
            else
            {
                if (!File::remove(entry.path))
                    return false;
            }
        }
    }
    
    auto wide_path = utf8_to_utf16(path.string());
    if (wide_path.is_empty() && !path.is_empty())
    {
        set_last_error(Error::InvalidPath);
        return false;
    }
    
    if (!RemoveDirectoryW(wide_path.data()))
    {
        set_last_error(win32_error_to_filesystem_error(GetLastError()));
        return false;
    }
    
    return true;
}

Path Directory::current()
{
    wchar_t buffer[MAX_PATH];
    DWORD length = GetCurrentDirectoryW(MAX_PATH, buffer);
    if (length == 0)
        return Path();
    return Path(utf16_to_utf8(buffer));
}

Path Directory::temp()
{
    wchar_t buffer[MAX_PATH];
    DWORD length = GetTempPathW(MAX_PATH, buffer);
    if (length == 0)
        return Path();
    return Path(utf16_to_utf8(buffer));
}

Path Directory::home()
{
    wchar_t buffer[MAX_PATH];
    if (GetEnvironmentVariableW(L"USERPROFILE", buffer, MAX_PATH) == 0)
        return Path();
    return Path(utf16_to_utf8(buffer));
}

Path Directory::executable()
{
    wchar_t buffer[MAX_PATH];
    DWORD size = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (size == 0)
        return Path();
    
    // Check if path was truncated
    if (size == MAX_PATH - 1)
    {
        // Try with larger buffer
        skr::Vector<wchar_t> large_buffer;
        large_buffer.resize_zeroed(32768);
        size = GetModuleFileNameW(nullptr, large_buffer.data(), static_cast<DWORD>(large_buffer.size()));
        if (size > 0 && size < large_buffer.size())
        {
            return Path(utf16_to_utf8(large_buffer.data()));
        }
    }
    
    return Path(utf16_to_utf8(buffer));
}

// ============================================================================
// DirectoryIterator 实现
// ============================================================================

struct DirectoryIteratorImpl
{
    HANDLE handle = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW find_data;
    bool first = true;
    Path search_path;
};

DirectoryIterator::DirectoryIterator(const Path& path)
{
    impl_ = new DirectoryIteratorImpl;
    
    auto* win_impl = static_cast<DirectoryIteratorImpl*>(impl_);
    win_impl->search_path = path / u8"*";
    
    auto wide_search_path = utf8_to_utf16(win_impl->search_path.string());
    if (wide_search_path.is_empty())
    {
        last_error_ = Error::InvalidPath;
        delete win_impl;
        impl_ = nullptr;
        return;
    }
    
    win_impl->handle = FindFirstFileW(
        wide_search_path.data(),
        &win_impl->find_data
    );
    
    if (win_impl->handle == INVALID_HANDLE_VALUE)
    {
        last_error_ = win32_error_to_filesystem_error(GetLastError());
        delete win_impl;
        impl_ = nullptr;
        return;
    }
    
    advance();
}

DirectoryIterator::~DirectoryIterator()
{
    if (impl_)
    {
        auto* win_impl = static_cast<DirectoryIteratorImpl*>(impl_);
        if (win_impl->handle != INVALID_HANDLE_VALUE)
            FindClose(win_impl->handle);
        delete win_impl;
    }
}

DirectoryIterator::DirectoryIterator(DirectoryIterator&& other) noexcept
    : impl_(other.impl_), current_(std::move(other.current_)), last_error_(other.last_error_)
{
    other.impl_ = nullptr;
    other.last_error_ = Error::None;
}

DirectoryIterator& DirectoryIterator::operator=(DirectoryIterator&& other) noexcept
{
    if (this != &other)
    {
        if (impl_)
        {
            auto* win_impl = static_cast<DirectoryIteratorImpl*>(impl_);
            if (win_impl->handle != INVALID_HANDLE_VALUE)
                FindClose(win_impl->handle);
            delete win_impl;
        }
        
        impl_ = other.impl_;
        current_ = std::move(other.current_);
        last_error_ = other.last_error_;
        
        other.impl_ = nullptr;
        other.last_error_ = Error::None;
    }
    return *this;
}

DirectoryIterator& DirectoryIterator::operator++()
{
    advance();
    return *this;
}

bool DirectoryIterator::operator==(const DirectoryIterator& other) const
{
    return (impl_ == nullptr) == (other.impl_ == nullptr);
}

bool DirectoryIterator::at_end() const
{
    return impl_ == nullptr;
}

void DirectoryIterator::advance()
{
    if (!impl_)
        return;
    
    auto* win_impl = static_cast<DirectoryIteratorImpl*>(impl_);
    
    bool found = false;
    while (!found)
    {
        if (win_impl->first)
        {
            win_impl->first = false;
            found = true;
        }
        else
        {
            if (!FindNextFileW(win_impl->handle, &win_impl->find_data))
            {
                // 到达末尾
                FindClose(win_impl->handle);
                delete win_impl;
                impl_ = nullptr;
                return;
            }
            found = true;
        }
        
        // 跳过 "." 和 ".."
        if (wcscmp(win_impl->find_data.cFileName, L".") == 0 ||
            wcscmp(win_impl->find_data.cFileName, L"..") == 0)
        {
            found = false;
        }
    }
    
    // 设置当前条目
    Path parent_path = win_impl->search_path.parent_directory();
    current_.path = parent_path / utf16_to_utf8(win_impl->find_data.cFileName);
    current_.type = win32_attributes_to_filetype(win_impl->find_data.dwFileAttributes);
}

// File seek/tell 实现
bool File::seek(int64_t offset, SeekOrigin origin)
{
    if (!handle_)
    {
        last_error_ = Error::NotFound;
        return false;
    }
    
    DWORD move_method;
    switch (origin)
    {
        case SeekOrigin::Begin: move_method = FILE_BEGIN; break;
        case SeekOrigin::Current: move_method = FILE_CURRENT; break;
        case SeekOrigin::End: move_method = FILE_END; break;
        default: last_error_ = Error::InvalidPath; return false;
    }
    
    LARGE_INTEGER li_offset;
    li_offset.QuadPart = offset;
    
    LARGE_INTEGER new_pos;
    if (!SetFilePointerEx(handle_, li_offset, &new_pos, move_method))
    {
        last_error_ = win32_error_to_filesystem_error(GetLastError());
        return false;
    }
    
    return true;
}

int64_t File::tell() const
{
    if (!handle_)
        return -1;
    
    LARGE_INTEGER offset;
    offset.QuadPart = 0;
    
    LARGE_INTEGER current_pos;
    if (!SetFilePointerEx(handle_, offset, &current_pos, FILE_CURRENT))
        return -1;
    
    return current_pos.QuadPart;
}

// ============================================================================
// Symlink 实现
// ============================================================================

bool Symlink::exists(const Path& path)
{
    set_last_error(Error::None);
    
    auto wide_path = utf8_to_utf16(path.string());
    if (wide_path.is_empty() && !path.is_empty())
    {
        set_last_error(Error::InvalidPath);
        return false;
    }
    
    DWORD attributes = GetFileAttributesW(wide_path.data());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        set_last_error(win32_error_to_filesystem_error(GetLastError()));
        return false;
    }
    return (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

bool Symlink::create(const Path& link, const Path& target)
{
    set_last_error(Error::None);
    
    auto wide_link = utf8_to_utf16(link.string());
    auto wide_target = utf8_to_utf16(target.string());
    if ((wide_link.is_empty() && !link.is_empty()) || (wide_target.is_empty() && !target.is_empty()))
    {
        set_last_error(Error::InvalidPath);
        return false;
    }
    
    // 检查目标是否为目录
    DWORD target_attrs = GetFileAttributesW(wide_target.data());
    DWORD flags = 0;
    
    if (target_attrs != INVALID_FILE_ATTRIBUTES && (target_attrs & FILE_ATTRIBUTE_DIRECTORY))
        flags = SYMBOLIC_LINK_FLAG_DIRECTORY;
    
    // Windows Vista 及以上版本支持符号链接
    if (!CreateSymbolicLinkW(wide_link.data(), wide_target.data(), flags))
    {
        set_last_error(win32_error_to_filesystem_error(GetLastError()));
        return false;
    }
    
    return true;
}

bool Symlink::remove(const Path& link)
{
    set_last_error(Error::None);
    
    auto wide_link = utf8_to_utf16(link.string());
    if (wide_link.is_empty() && !link.is_empty())
    {
        set_last_error(Error::InvalidPath);
        return false;
    }
    
    // 检查是否为符号链接
    DWORD attrs = GetFileAttributesW(wide_link.data());
    if (attrs == INVALID_FILE_ATTRIBUTES)
    {
        set_last_error(win32_error_to_filesystem_error(GetLastError()));
        return false;
    }
    
    if (!(attrs & FILE_ATTRIBUTE_REPARSE_POINT))
    {
        set_last_error(Error::NotAFile);
        return false;
    }
    
    // 删除符号链接（目录链接用 RemoveDirectory，文件链接用 DeleteFile）
    if (attrs & FILE_ATTRIBUTE_DIRECTORY)
    {
        if (!RemoveDirectoryW(wide_link.data()))
        {
            set_last_error(win32_error_to_filesystem_error(GetLastError()));
            return false;
        }
    }
    else
    {
        if (!DeleteFileW(wide_link.data()))
        {
            set_last_error(win32_error_to_filesystem_error(GetLastError()));
            return false;
        }
    }
    
    return true;
}

Path Symlink::read(const Path& link)
{
    set_last_error(Error::None);
    
    // Windows 符号链接实现较复杂，这里简化处理
    set_last_error(Error::NotSupported);
    return Path();
}

} // namespace skr::fs