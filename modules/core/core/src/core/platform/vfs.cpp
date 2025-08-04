#include "SkrCore/platform/vfs.h"
#include <SkrOS/filesystem.hpp>
#include "SkrContainers/string.hpp"

void skr_vfs_get_parent_path(const char8_t* path, char8_t* output)
{
    const skr::Path p{skr::String(path)};
    auto parent = p.parent_directory();
    std::strcpy((char*)output, (const char*)parent.string().data());
}

void skr_vfs_append_path_component(const char8_t* path, const char8_t* component, char8_t* output)
{
    const skr::Path p{skr::String(path)};
    const auto appended = p / skr::String(component);
    std::strcpy((char*)output, (const char*)appended.string().data());
}

void skr_vfs_append_path_extension(const char8_t* path, const char8_t* extension, char8_t* output)
{
    skr::String p(path);
    if (extension[0] != u8'.')
    {
        p.append(u8".");
    }
    p.append(extension);
    std::strcpy((char*)output, p.c_str_raw());
}

skr_vfile_t* skr_vfs_fopen(skr_vfs_t* fs, const char8_t* path, ESkrFileMode mode, ESkrFileCreation creation) SKR_NOEXCEPT
{
    return fs->procs.fopen(fs, path, mode, creation);
}

size_t skr_vfs_fread(skr_vfile_t* file, void* out_buffer, size_t offset, size_t byte_count) SKR_NOEXCEPT
{
    return file->fs->procs.fread(file, out_buffer, offset, byte_count);
}

size_t skr_vfs_fwrite(skr_vfile_t* file, const void* in_buffer, size_t offset, size_t byte_count) SKR_NOEXCEPT
{
    return file->fs->procs.fwrite(file, in_buffer, offset, byte_count);
}

int64_t skr_vfs_fsize(const skr_vfile_t* file) SKR_NOEXCEPT
{
    return file->fs->procs.fsize(file);
}

bool skr_vfs_fclose(skr_vfile_t* file) SKR_NOEXCEPT
{
    return file->fs->procs.fclose(file);
}

// File system operations
bool skr_vfs_fexists(skr_vfs_t* fs, const char8_t* path) SKR_NOEXCEPT
{
    return fs->procs.fexists ? fs->procs.fexists(fs, path) : false;
}

bool skr_vfs_fis_directory(skr_vfs_t* fs, const char8_t* path) SKR_NOEXCEPT
{
    return fs->procs.fis_directory ? fs->procs.fis_directory(fs, path) : false;
}

bool skr_vfs_fremove(skr_vfs_t* fs, const char8_t* path) SKR_NOEXCEPT
{
    return fs->procs.fremove ? fs->procs.fremove(fs, path) : false;
}

bool skr_vfs_frename(skr_vfs_t* fs, const char8_t* from, const char8_t* to) SKR_NOEXCEPT
{
    return fs->procs.frename ? fs->procs.frename(fs, from, to) : false;
}

bool skr_vfs_fcopy(skr_vfs_t* fs, const char8_t* from, const char8_t* to) SKR_NOEXCEPT
{
    return fs->procs.fcopy ? fs->procs.fcopy(fs, from, to) : false;
}

int64_t skr_vfs_fmtime(skr_vfs_t* fs, const char8_t* path) SKR_NOEXCEPT
{
    return fs->procs.fmtime ? fs->procs.fmtime(fs, path) : -1;
}

// Directory operations
bool skr_vfs_mkdir(skr_vfs_t* fs, const char8_t* path) SKR_NOEXCEPT
{
    return fs->procs.mkdir ? fs->procs.mkdir(fs, path) : false;
}

bool skr_vfs_rmdir(skr_vfs_t* fs, const char8_t* path) SKR_NOEXCEPT
{
    return fs->procs.rmdir ? fs->procs.rmdir(fs, path) : false;
}