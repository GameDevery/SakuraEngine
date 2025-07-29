#include "SkrOS/filesystem.hpp"
#include "../winheaders.h"
#include <direct.h>
#include <io.h>
#include <sys/stat.h>

namespace skr::fs
{

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
    
    DWORD attributes = GetFileAttributesA(reinterpret_cast<const char*>(path.string().data()));
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
    
    WIN32_FILE_ATTRIBUTE_DATA fileData;
    if (!GetFileAttributesExA(reinterpret_cast<const char*>(path.string().data()), 
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
    
    if (!DeleteFileA(reinterpret_cast<const char*>(path.string().data())))
    {
        set_last_error(win32_error_to_filesystem_error(GetLastError()));
        return false;
    }
    return true;
}

bool File::copy(const Path& from, const Path& to, CopyOptions options)
{
    set_last_error(Error::None);
    
    BOOL overwrite = (static_cast<uint32_t>(options) & static_cast<uint32_t>(CopyOptions::OverwriteExisting)) != 0;
    BOOL fail_if_exists = !overwrite;
    
    if (!CopyFileA(reinterpret_cast<const char*>(from.string().data()),
                   reinterpret_cast<const char*>(to.string().data()),
                   fail_if_exists))
    {
        set_last_error(win32_error_to_filesystem_error(GetLastError()));
        return false;
    }
    
    return true;
}

uint64_t File::size(const Path& path)
{
    auto info = get_info(path);
    return info.exists() ? info.size : 0;
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
    
    if (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OpenMode::Create))
    {
        if (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OpenMode::Exclusive))
            creation = CREATE_NEW;
        else if (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OpenMode::Truncate))
            creation = CREATE_ALWAYS;
        else
            creation = OPEN_ALWAYS;
    }
    else
    {
        if (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OpenMode::Truncate))
            creation = TRUNCATE_EXISTING;
        else
            creation = OPEN_EXISTING;
    }
    
    handle_ = CreateFileA(
        reinterpret_cast<const char*>(path.string().data()),
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
    
    DWORD attributes = GetFileAttributesA(reinterpret_cast<const char*>(path.string().data()));
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
        if (!parent.empty() && !exists(parent))
        {
            if (!create(parent, true))
                return false;
        }
    }
    
    if (!CreateDirectoryA(reinterpret_cast<const char*>(path.string().data()), nullptr))
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
    
    if (!RemoveDirectoryA(reinterpret_cast<const char*>(path.string().data())))
    {
        set_last_error(win32_error_to_filesystem_error(GetLastError()));
        return false;
    }
    
    return true;
}

Path Directory::current()
{
    char buffer[MAX_PATH];
    DWORD length = GetCurrentDirectoryA(MAX_PATH, buffer);
    if (length == 0)
        return Path();
    return Path(skr::String(reinterpret_cast<const char8_t*>(buffer), length));
}

Path Directory::temp()
{
    char buffer[MAX_PATH];
    DWORD length = GetTempPathA(MAX_PATH, buffer);
    if (length == 0)
        return Path();
    return Path(skr::String(reinterpret_cast<const char8_t*>(buffer), length));
}

Path Directory::home()
{
    char buffer[MAX_PATH];
    if (GetEnvironmentVariableA("USERPROFILE", buffer, MAX_PATH) == 0)
        return Path();
    return Path(skr::String(reinterpret_cast<const char8_t*>(buffer)));
}

// ============================================================================
// DirectoryIterator 实现
// ============================================================================

struct DirectoryIteratorImpl
{
    HANDLE handle = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA find_data;
    bool first = true;
    Path search_path;
};

DirectoryIterator::DirectoryIterator(const Path& path)
{
    impl_ = new DirectoryIteratorImpl;
    
    auto* win_impl = static_cast<DirectoryIteratorImpl*>(impl_);
    win_impl->search_path = path / u8"*";
    win_impl->handle = FindFirstFileA(
        reinterpret_cast<const char*>(win_impl->search_path.string().data()),
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
            if (!FindNextFileA(win_impl->handle, &win_impl->find_data))
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
        if (strcmp(win_impl->find_data.cFileName, ".") == 0 ||
            strcmp(win_impl->find_data.cFileName, "..") == 0)
        {
            found = false;
        }
    }
    
    // 设置当前条目
    Path parent_path = win_impl->search_path.parent_directory();
    current_.path = parent_path / skr::String(reinterpret_cast<const char8_t*>(win_impl->find_data.cFileName));
    current_.type = win32_attributes_to_filetype(win_impl->find_data.dwFileAttributes);
}

// ============================================================================
// Symlink 实现
// ============================================================================

bool Symlink::exists(const Path& path)
{
    set_last_error(Error::None);
    
    DWORD attributes = GetFileAttributesA(reinterpret_cast<const char*>(path.string().data()));
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        set_last_error(win32_error_to_filesystem_error(GetLastError()));
        return false;
    }
    return (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

Path Symlink::read(const Path& link)
{
    set_last_error(Error::None);
    
    // Windows 符号链接实现较复杂，这里简化处理
    set_last_error(Error::NotSupported);
    return Path();
}

} // namespace skr::fs