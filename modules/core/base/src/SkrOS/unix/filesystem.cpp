#include "SkrOS/filesystem.hpp"
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace skr::fs
{

// 声明在 build.filesystem.cpp 中定义的函数
extern Error get_last_error();
extern void set_last_error(Error error);
extern Error g_last_error;

// 错误码转换
Error posix_error_to_filesystem_error(int error)
{
    switch (error) {
        case 0: return Error::None;
        case ENOENT: return Error::NotFound;
        case EACCES:
        case EPERM: return Error::AccessDenied;
        case EEXIST: return Error::AlreadyExists;
        case ENOTDIR: return Error::NotADirectory;
        case EISDIR: return Error::NotAFile;
        case ENOTEMPTY: return Error::DirectoryNotEmpty;
        case ENAMETOOLONG:
        case EINVAL: return Error::InvalidPath;
        case EMFILE:
        case ENFILE: return Error::TooManyOpenFiles;
        case ENOSPC: return Error::DiskFull;
        case EROFS: return Error::ReadOnlyFileSystem;
        case ENOTSUP: return Error::NotSupported;
        default: return Error::Unknown;
    }
}

FileType posix_mode_to_filetype(mode_t mode)
{
    if (S_ISREG(mode)) return FileType::Regular;
    if (S_ISDIR(mode)) return FileType::Directory;
    if (S_ISLNK(mode)) return FileType::Symlink;
    if (S_ISBLK(mode)) return FileType::Block;
    if (S_ISCHR(mode)) return FileType::Character;
    if (S_ISFIFO(mode)) return FileType::Fifo;
    if (S_ISSOCK(mode)) return FileType::Socket;
    return FileType::Unknown;
}

Permissions posix_mode_to_permissions(mode_t mode)
{
    return static_cast<Permissions>(mode & 07777);
}

// ============================================================================
// File 实现
// ============================================================================

bool File::exists(const Path& path)
{
    set_last_error(Error::None);
    
    struct stat st;
    if (stat(reinterpret_cast<const char*>(path.string().data()), &st) != 0)
    {
        set_last_error(posix_error_to_filesystem_error(errno));
        return false;
    }
    return S_ISREG(st.st_mode);
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
    
    struct stat st;
    if (stat(reinterpret_cast<const char*>(path.string().data()), &st) != 0)
    {
        set_last_error(posix_error_to_filesystem_error(errno));
        return info;
    }
    
    info.type = posix_mode_to_filetype(st.st_mode);
    info.size = st.st_size;
    info.permissions = posix_mode_to_permissions(st.st_mode);
    
    // 转换时间戳到 FileTime (100纳秒间隔)
    info.creation_time = unix_to_filetime(st.st_ctime);
    info.last_write_time = unix_to_filetime(st.st_mtime);
    info.last_access_time = unix_to_filetime(st.st_atime);
    
    info.platform_attrs.unix_mode = st.st_mode;
    
    return info;
}

Error File::get_last_error()
{
    return g_last_error;
}

bool File::remove(const Path& path)
{
    set_last_error(Error::None);
    
    if (unlink(reinterpret_cast<const char*>(path.string().data())) != 0)
    {
        set_last_error(posix_error_to_filesystem_error(errno));
        return false;
    }
    return true;
}

bool File::copy(const Path& from, const Path& to, CopyOptions options)
{
    set_last_error(Error::None);
    
    // Open source file
    int src_fd = ::open(reinterpret_cast<const char*>(from.string().data()), O_RDONLY);
    if (src_fd == -1)
    {
        set_last_error(posix_error_to_filesystem_error(errno));
        return false;
    }
    
    // Get source file info
    struct stat src_stat;
    if (fstat(src_fd, &src_stat) != 0)
    {
        ::close(src_fd);
        set_last_error(posix_error_to_filesystem_error(errno));
        return false;
    }
    
    // Open destination file
    int flags = O_WRONLY | O_CREAT;
    if (static_cast<uint32_t>(options) & static_cast<uint32_t>(CopyOptions::OverwriteExisting))
        flags |= O_TRUNC;
    else
        flags |= O_EXCL;
    
    int dst_fd = ::open(reinterpret_cast<const char*>(to.string().data()), flags, src_stat.st_mode);
    if (dst_fd == -1)
    {
        ::close(src_fd);
        set_last_error(posix_error_to_filesystem_error(errno));
        return false;
    }
    
    // Copy data
    char buffer[8192];
    ssize_t bytes_read;
    bool success = true;
    
    while ((bytes_read = ::read(src_fd, buffer, sizeof(buffer))) > 0)
    {
        ssize_t bytes_written = ::write(dst_fd, buffer, bytes_read);
        if (bytes_written != bytes_read)
        {
            success = false;
            set_last_error(posix_error_to_filesystem_error(errno));
            break;
        }
    }
    
    if (bytes_read == -1)
    {
        success = false;
        set_last_error(posix_error_to_filesystem_error(errno));
    }
    
    ::close(src_fd);
    ::close(dst_fd);
    
    return success;
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
    
    int flags = 0;
    
    if ((static_cast<uint32_t>(mode) & static_cast<uint32_t>(OpenMode::Read)) && 
        (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OpenMode::Write)))
        flags |= O_RDWR;
    else if (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OpenMode::Write))
        flags |= O_WRONLY;
    else
        flags |= O_RDONLY;
    
    if (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OpenMode::Create)) flags |= O_CREAT;
    if (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OpenMode::Exclusive)) flags |= O_EXCL;
    if (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OpenMode::Truncate)) flags |= O_TRUNC;
    if (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OpenMode::Append)) flags |= O_APPEND;
    
    int fd = ::open(reinterpret_cast<const char*>(path.string().data()), flags, 0644);
    if (fd == -1)
    {
        last_error_ = posix_error_to_filesystem_error(errno);
        return false;
    }
    
    handle_ = reinterpret_cast<void*>(static_cast<intptr_t>(fd));
    return true;
}

void File::close()
{
    if (handle_)
    {
        int fd = static_cast<int>(reinterpret_cast<intptr_t>(handle_));
        ::close(fd);
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
    
    int fd = static_cast<int>(reinterpret_cast<intptr_t>(handle_));
    ssize_t result = ::read(fd, buffer, size);
    if (result < 0)
    {
        last_error_ = posix_error_to_filesystem_error(errno);
        return 0;
    }
    return static_cast<size_t>(result);
}

size_t File::write(const void* buffer, size_t size)
{
    if (!handle_)
    {
        last_error_ = Error::NotFound;
        return 0;
    }
    
    int fd = static_cast<int>(reinterpret_cast<intptr_t>(handle_));
    ssize_t result = ::write(fd, buffer, size);
    if (result < 0)
    {
        last_error_ = posix_error_to_filesystem_error(errno);
        return 0;
    }
    return static_cast<size_t>(result);
}

bool File::seek(int64_t offset, SeekOrigin origin)
{
    if (!handle_)
    {
        last_error_ = Error::InvalidPath;
        return false;
    }
    
    int fd = static_cast<int>(reinterpret_cast<intptr_t>(handle_));
    int whence = SEEK_SET;
    switch (origin)
    {
        case SeekOrigin::Begin: whence = SEEK_SET; break;
        case SeekOrigin::Current: whence = SEEK_CUR; break;
        case SeekOrigin::End: whence = SEEK_END; break;
    }
    
    if (lseek(fd, offset, whence) == -1)
    {
        last_error_ = posix_error_to_filesystem_error(errno);
        return false;
    }
    
    last_error_ = Error::None;
    return true;
}

int64_t File::tell() const
{
    if (!handle_)
        return -1;
    
    int fd = static_cast<int>(reinterpret_cast<intptr_t>(handle_));
    off_t pos = lseek(fd, 0, SEEK_CUR);
    if (pos == -1)
    {
        const_cast<File*>(this)->last_error_ = posix_error_to_filesystem_error(errno);
        return -1;
    }
    
    return static_cast<int64_t>(pos);
}

int64_t File::get_size() const
{
    if (!handle_)
        return -1;
    
    int fd = static_cast<int>(reinterpret_cast<intptr_t>(handle_));
    struct stat st;
    if (fstat(fd, &st) != 0)
        return -1;
    return st.st_size;
}

// ============================================================================
// Directory 实现
// ============================================================================

bool Directory::exists(const Path& path)
{
    set_last_error(Error::None);
    
    struct stat st;
    if (stat(reinterpret_cast<const char*>(path.string().data()), &st) != 0)
    {
        set_last_error(posix_error_to_filesystem_error(errno));
        return false;
    }
    return S_ISDIR(st.st_mode);
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
    
    if (mkdir(reinterpret_cast<const char*>(path.string().data()), 0755) != 0)
    {
        set_last_error(posix_error_to_filesystem_error(errno));
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
    
    if (rmdir(reinterpret_cast<const char*>(path.string().data())) != 0)
    {
        set_last_error(posix_error_to_filesystem_error(errno));
        return false;
    }
    
    return true;
}

Path Directory::current()
{
    char buffer[PATH_MAX];
    if (getcwd(buffer, sizeof(buffer)) == nullptr)
        return Path();
    return Path(skr::String(reinterpret_cast<const char8_t*>(buffer)));
}

Path Directory::temp()
{
    const char* tmp = getenv("TMPDIR");
    if (!tmp) tmp = getenv("TMP");
    if (!tmp) tmp = getenv("TEMP");
    if (!tmp) tmp = "/tmp";
    return Path(skr::String(reinterpret_cast<const char8_t*>(tmp)));
}

Path Directory::home()
{
    const char* home = getenv("HOME");
    if (!home)
        return Path();
    return Path(skr::String(reinterpret_cast<const char8_t*>(home)));
}

Path Directory::executable()
{
#ifdef __APPLE__
    // macOS-specific implementation using _NSGetExecutablePath
    char buffer[PATH_MAX];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0)
    {
        // Resolve symlinks to get the real path
        char resolved[PATH_MAX];
        if (realpath(buffer, resolved) != nullptr)
        {
            return Path(skr::String(reinterpret_cast<const char8_t*>(resolved)));
        }
    }
    return Path();
#elif defined(__linux__)
    // Linux-specific implementation using /proc/self/exe
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1)
    {
        buffer[len] = '\0';
        return Path(skr::String(reinterpret_cast<const char8_t*>(buffer)));
    }
    return Path();
#else
    // Generic Unix fallback - not reliable
    return Path();
#endif
}

// ============================================================================
// DirectoryIterator 实现
// ============================================================================

struct DirectoryIteratorImpl
{
    DIR* dir = nullptr;
    Path dir_path;
};

DirectoryIterator::DirectoryIterator(const Path& path)
{
    impl_ = new DirectoryIteratorImpl;
    
    auto* posix_impl = static_cast<DirectoryIteratorImpl*>(impl_);
    posix_impl->dir_path = path;
    posix_impl->dir = opendir(reinterpret_cast<const char*>(path.string().data()));
    
    if (!posix_impl->dir)
    {
        last_error_ = posix_error_to_filesystem_error(errno);
        delete posix_impl;
        impl_ = nullptr;
        return;
    }
    
    advance();
}

DirectoryIterator::~DirectoryIterator()
{
    if (impl_)
    {
        auto* posix_impl = static_cast<DirectoryIteratorImpl*>(impl_);
        if (posix_impl->dir)
            closedir(posix_impl->dir);
        delete posix_impl;
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
            auto* posix_impl = static_cast<DirectoryIteratorImpl*>(impl_);
            if (posix_impl->dir)
                closedir(posix_impl->dir);
            delete posix_impl;
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
    
    auto* posix_impl = static_cast<DirectoryIteratorImpl*>(impl_);
    
    struct dirent* entry = nullptr;
    while ((entry = readdir(posix_impl->dir)) != nullptr)
    {
        // 跳过 "." 和 ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        current_.path = posix_impl->dir_path / skr::String(reinterpret_cast<const char8_t*>(entry->d_name));
        
        // 获取文件类型
#ifdef _DIRENT_HAVE_D_TYPE
        switch (entry->d_type)
        {
            case DT_REG: current_.type = FileType::Regular; break;
            case DT_DIR: current_.type = FileType::Directory; break;
            case DT_LNK: current_.type = FileType::Symlink; break;
            case DT_BLK: current_.type = FileType::Block; break;
            case DT_CHR: current_.type = FileType::Character; break;
            case DT_FIFO: current_.type = FileType::Fifo; break;
            case DT_SOCK: current_.type = FileType::Socket; break;
            default: 
                current_.type = FileType::Unknown;
                // 回退到 stat
                auto info = File::get_info(current_.path);
                current_.type = info.type;
                break;
        }
#else
        // 回退到 stat 获取类型
        auto info = File::get_info(current_.path);
        current_.type = info.type;
#endif
        return;
    }
    
    // 到达末尾
    closedir(posix_impl->dir);
    delete posix_impl;
    impl_ = nullptr;
}

// ============================================================================
// Symlink 实现
// ============================================================================

bool Symlink::exists(const Path& path)
{
    set_last_error(Error::None);
    
    struct stat st;
    if (lstat(reinterpret_cast<const char*>(path.string().data()), &st) != 0)
    {
        set_last_error(posix_error_to_filesystem_error(errno));
        return false;
    }
    return S_ISLNK(st.st_mode);
}

bool Symlink::create(const Path& link, const Path& target)
{
    set_last_error(Error::None);
    
    if (symlink(reinterpret_cast<const char*>(target.string().data()), 
                reinterpret_cast<const char*>(link.string().data())) != 0)
    {
        set_last_error(posix_error_to_filesystem_error(errno));
        return false;
    }
    return true;
}

bool Symlink::remove(const Path& link)
{
    set_last_error(Error::None);
    
    if (unlink(reinterpret_cast<const char*>(link.string().data())) != 0)
    {
        set_last_error(posix_error_to_filesystem_error(errno));
        return false;
    }
    return true;
}

Path Symlink::read(const Path& link)
{
    set_last_error(Error::None);
    
    char buffer[PATH_MAX];
    ssize_t len = readlink(reinterpret_cast<const char*>(link.string().data()), buffer, PATH_MAX - 1);
    if (len == -1)
    {
        set_last_error(posix_error_to_filesystem_error(errno));
        return Path();
    }
    buffer[len] = '\0';
    return Path(skr::String(reinterpret_cast<const char8_t*>(buffer)));
}

} // namespace skr::fs