#include "SkrOS/filesystem.hpp"
#include "SkrTestFramework/framework.hpp"
#include "SkrOS/thread.h"
#include <cstring>
#include <atomic>

// Helper function to create a unique test directory
static skr::Path create_test_directory(const char8_t* name)
{
    auto base = skr::fs::Directory::temp() / u8"skr_fs_test";
    skr::fs::Directory::create(base, true);

    static std::atomic<uint64_t> counter{ 0 };
    auto timestamp = counter.fetch_add(1);
    auto unique_name = skr::format(u8"{}_{}", name, timestamp);
    auto test_dir = base / unique_name;

    skr::fs::Directory::create(test_dir, true);
    return test_dir;
}

// Helper to clean up test directory recursively
static void cleanup_test_directory(const skr::Path& dir)
{
    if (skr::fs::Directory::exists(dir))
    {
        skr::fs::Directory::remove(dir, true);
    }
}

TEST_CASE("FileSystem: File Static Methods - Text Operations")
{
    auto test_dir = create_test_directory(u8"file_static_text");
    auto test_file = test_dir / u8"test.txt";

    SUBCASE("Write and read text with UTF-8")
    {
        // Test UTF-8 content with special characters
        const char8_t* test_content = u8"Hello, 世界! 🌍\nLine 2\rLine 3\r\nLine 4";

        CHECK(skr::fs::File::write_all_text(test_file, test_content));
        CHECK(skr::fs::File::exists(test_file));

        skr::String read_content;
        CHECK(skr::fs::File::read_all_text(test_file, read_content));
        CHECK(read_content == test_content);
    }

    SUBCASE("Write and read empty file")
    {
        CHECK(skr::fs::File::write_all_text(test_file, u8""));
        CHECK(skr::fs::File::exists(test_file));
        CHECK(skr::fs::File::size(test_file) == 0);

        skr::String content;
        CHECK(skr::fs::File::read_all_text(test_file, content));
        CHECK(content.is_empty());
    }

    cleanup_test_directory(test_dir);
}

TEST_CASE("FileSystem: File Static Methods - Binary Operations")
{
    auto test_dir = create_test_directory(u8"file_static_binary");
    auto test_file = test_dir / u8"test.bin";

    SUBCASE("Write and read bytes")
    {
        skr::Vector<uint8_t> test_bytes = { 0x00, 0xFF, 0x42, 0xDE, 0xAD, 0xBE, 0xEF };

        CHECK(skr::fs::File::write_all_bytes(test_file, test_bytes));
        CHECK(skr::fs::File::exists(test_file));
        CHECK(skr::fs::File::size(test_file) == test_bytes.size());

        skr::Vector<uint8_t> read_bytes;
        CHECK(skr::fs::File::read_all_bytes(test_file, read_bytes));
        CHECK(read_bytes.size() == test_bytes.size());
        CHECK(std::memcmp(read_bytes.data(), test_bytes.data(), test_bytes.size()) == 0);
    }

    SUBCASE("Write and read large binary data")
    {
        // Create 1MB of test data
        const size_t size = 1024 * 1024;
        skr::Vector<uint8_t> large_data;
        large_data.resize_zeroed(size);

        // Fill with pattern
        for (size_t i = 0; i < size; i++)
        {
            large_data[i] = static_cast<uint8_t>(i % 256);
        }

        CHECK(skr::fs::File::write_all_bytes(test_file, large_data));
        CHECK(skr::fs::File::size(test_file) == size);

        skr::Vector<uint8_t> read_data;
        CHECK(skr::fs::File::read_all_bytes(test_file, read_data));
        CHECK(read_data.size() == size);
        CHECK(std::memcmp(read_data.data(), large_data.data(), size) == 0);
    }

    cleanup_test_directory(test_dir);
}

TEST_CASE("FileSystem: File Static Methods - Line Operations")
{
    auto test_dir = create_test_directory(u8"file_static_lines");
    auto test_file = test_dir / u8"test.txt";

    SUBCASE("Write and read text with newlines")
    {
        skr::String test_content = u8"Line 1\n"
                                   u8"Line 2 with 中文\n"
                                   u8"Line 3 with emoji 😊\n"
                                   u8"\n" // Empty line
                                   u8"Line 5";

        CHECK(skr::fs::File::write_all_text(test_file, test_content));

        skr::String read_content;
        CHECK(skr::fs::File::read_all_text(test_file, read_content));
        CHECK(read_content == test_content);
    }

    SUBCASE("Line endings handling")
    {
        // Write lines with different line endings
        CHECK(skr::fs::File::write_all_text(test_file, u8"Line1\nLine2\r\nLine3\rLine4"));

        skr::String content;
        CHECK(skr::fs::File::read_all_text(test_file, content));
        // Content should preserve all line endings
        CHECK(content == u8"Line1\nLine2\r\nLine3\rLine4");
    }

    cleanup_test_directory(test_dir);
}

TEST_CASE("FileSystem: File Copy Operations")
{
    auto test_dir = create_test_directory(u8"file_copy");
    auto source = test_dir / u8"source.txt";
    auto dest = test_dir / u8"dest.txt";

    SUBCASE("Basic copy")
    {
        CHECK(skr::fs::File::write_all_text(source, u8"Test content"));

        CHECK(skr::fs::File::copy(source, dest, skr::fs::CopyOptions::None));
        CHECK(skr::fs::File::exists(dest));

        skr::String content;
        CHECK(skr::fs::File::read_all_text(dest, content));
        CHECK(content == u8"Test content");
    }

    SUBCASE("Copy with overwrite")
    {
        CHECK(skr::fs::File::write_all_text(source, u8"Source content"));
        CHECK(skr::fs::File::write_all_text(dest, u8"Dest content"));

        // Without overwrite flag should fail
        CHECK(!skr::fs::File::copy(source, dest, skr::fs::CopyOptions::None));

        // With overwrite should succeed
        CHECK(skr::fs::File::copy(source, dest, skr::fs::CopyOptions::OverwriteExisting));

        skr::String content;
        CHECK(skr::fs::File::read_all_text(dest, content));
        CHECK(content == u8"Source content");
    }

    SUBCASE("Copy with skip existing")
    {
        CHECK(skr::fs::File::write_all_text(source, u8"New content"));
        CHECK(skr::fs::File::write_all_text(dest, u8"Existing content"));

        CHECK(skr::fs::File::copy(source, dest, skr::fs::CopyOptions::SkipExisting));

        skr::String content;
        CHECK(skr::fs::File::read_all_text(dest, content));
        CHECK(content == u8"Existing content"); // Should not change
    }

    cleanup_test_directory(test_dir);
}

TEST_CASE("FileSystem: File Info and Attributes")
{
    auto test_dir = create_test_directory(u8"file_info");
    auto test_file = test_dir / u8"test.txt";

    SUBCASE("Basic file info")
    {
        CHECK(skr::fs::File::write_all_text(test_file, u8"Test content"));

        auto info = skr::fs::File::get_info(test_file);
        CHECK(info.exists());
        CHECK(info.is_regular_file());
        CHECK(!info.is_directory());
        CHECK(!info.is_symlink());
        CHECK(info.size == 12); // "Test content" is 12 bytes
    }

    SUBCASE("Timestamps")
    {
        CHECK(skr::fs::File::write_all_text(test_file, u8"Test"));

        auto info = skr::fs::File::get_info(test_file);
        CHECK(info.creation_time != 0);
        CHECK(info.last_write_time != 0);
        CHECK(info.last_access_time != 0);

        // Modify file and check if write time changes
        skr_thread_sleep(10);
        CHECK(skr::fs::File::write_all_text(test_file, u8"Updated"));

        auto new_info = skr::fs::File::get_info(test_file);
        CHECK(new_info.last_write_time > info.last_write_time);
    }

    SUBCASE("Non-existent file")
    {
        auto non_existent = test_dir / u8"non_existent.txt";

        CHECK(!skr::fs::File::exists(non_existent));
        CHECK(skr::fs::File::size(non_existent) == static_cast<uint64_t>(-1));

        auto info = skr::fs::File::get_info(non_existent);
        CHECK(!info.exists());
    }

    cleanup_test_directory(test_dir);
}

TEST_CASE("FileSystem: File Handle Operations")
{
    auto test_dir = create_test_directory(u8"file_handle");
    auto test_file = test_dir / u8"handle_test.txt";

    SUBCASE("Read and write modes")
    {
        // Write only
        {
            skr::fs::File file;
            CHECK(file.open(test_file, skr::fs::OpenMode::Write | skr::fs::OpenMode::Create | skr::fs::OpenMode::Truncate));
            CHECK(file.is_open());

            const char8_t* data = u8"Write only test";
            CHECK(file.write(data, std::strlen((const char*)data)) > 0);

            // Should not be able to read in write-only mode
            char8_t buffer[100];
            CHECK(file.read(buffer, sizeof(buffer)) == 0);

            file.close();
        }

        // Read only
        {
            skr::fs::File file;
            CHECK(file.open(test_file, skr::fs::OpenMode::ReadOnly));

            char8_t buffer[100] = { 0 };
            size_t read = file.read(buffer, sizeof(buffer) - 1);
            CHECK(read > 0);
            CHECK(skr::StringView(buffer) == u8"Write only test");

            // Should not be able to write in read-only mode
            CHECK(file.write(u8"test", 4) == 0);

            file.close();
        }
    }

    SUBCASE("Seek and tell")
    {
        const char8_t* test_data = u8"0123456789ABCDEF";
        CHECK(skr::fs::File::write_all_text(test_file, test_data));

        skr::fs::File file;
        CHECK(file.open(test_file, skr::fs::OpenMode::ReadOnly));

        // Test tell
        CHECK(file.tell() == 0);

        // Test seek from beginning
        CHECK(file.seek(5, skr::fs::SeekOrigin::Begin));
        CHECK(file.tell() == 5);

        char8_t buffer[5] = { 0 };
        CHECK(file.read(buffer, 4) == 4);
        CHECK(std::memcmp(buffer, u8"5678", 4) == 0);
        CHECK(file.tell() == 9);

        // Test seek from current
        CHECK(file.seek(-4, skr::fs::SeekOrigin::Current));
        CHECK(file.tell() == 5);

        // Test seek from end
        CHECK(file.seek(-6, skr::fs::SeekOrigin::End));
        CHECK(file.tell() == 10);

        CHECK(file.read(buffer, 4) == 4);
        CHECK(std::memcmp(buffer, u8"ABCD", 4) == 0);

        file.close();
    }

    SUBCASE("Append mode")
    {
        CHECK(skr::fs::File::write_all_text(test_file, u8"Initial"));

        skr::fs::File file;
        CHECK(file.open(test_file, skr::fs::OpenMode::Write | skr::fs::OpenMode::Append));

        const char8_t* append_data = u8" Appended";
        CHECK(file.write(append_data, std::strlen((const char*)append_data)) > 0);

        file.close();

        skr::String content;
        CHECK(skr::fs::File::read_all_text(test_file, content));
        CHECK(content == u8"Initial Appended");
    }

    cleanup_test_directory(test_dir);
}

TEST_CASE("FileSystem: Directory Operations")
{
    auto test_dir = create_test_directory(u8"dir_ops");

    SUBCASE("Create and remove directories")
    {
        auto single_dir = test_dir / u8"single";
        CHECK(skr::fs::Directory::create(single_dir));
        CHECK(skr::fs::Directory::exists(single_dir));
        CHECK(skr::fs::Directory::is_directory(single_dir));

        CHECK(skr::fs::Directory::remove(single_dir, false));
        CHECK(!skr::fs::Directory::exists(single_dir));
    }

    SUBCASE("Nested directory creation")
    {
        auto nested = test_dir / u8"level1" / u8"level2" / u8"level3";

        // Non-recursive should fail
        CHECK(!skr::fs::Directory::create(nested, false));

        // Recursive should succeed
        CHECK(skr::fs::Directory::create(nested, true));
        CHECK(skr::fs::Directory::exists(nested));
    }

    SUBCASE("Directory with contents removal")
    {
        auto dir_with_files = test_dir / u8"with_files";
        CHECK(skr::fs::Directory::create(dir_with_files));

        // Create some files
        CHECK(skr::fs::File::write_all_text(dir_with_files / u8"file1.txt", u8"content1"));
        CHECK(skr::fs::File::write_all_text(dir_with_files / u8"file2.txt", u8"content2"));

        // Create subdirectory
        auto subdir = dir_with_files / u8"subdir";
        CHECK(skr::fs::Directory::create(subdir));
        CHECK(skr::fs::File::write_all_text(subdir / u8"subfile.txt", u8"subcontent"));

        // Non-recursive removal should fail
        CHECK(!skr::fs::Directory::remove(dir_with_files, false));

        // Recursive removal should succeed
        CHECK(skr::fs::Directory::remove(dir_with_files, true));
        CHECK(!skr::fs::Directory::exists(dir_with_files));
    }

    cleanup_test_directory(test_dir);
}

TEST_CASE("FileSystem: Special Directories")
{
    SUBCASE("System directories")
    {
        auto temp = skr::fs::Directory::temp();
        CHECK(!temp.is_empty());
        CHECK(skr::fs::Directory::exists(temp));

        auto current = skr::fs::Directory::current();
        CHECK(!current.is_empty());
        CHECK(skr::fs::Directory::exists(current));

        auto home = skr::fs::Directory::home();
        CHECK(!home.is_empty());
        CHECK(skr::fs::Directory::exists(home));
    }

    SUBCASE("Executable paths")
    {
        auto executable = skr::fs::Directory::executable();
        CHECK(!executable.is_empty());
        // executable might be a file path, not a directory
    }

    SUBCASE("Change directory")
    {
        auto original_dir = skr::fs::Directory::current();
        auto temp_dir = skr::fs::Directory::temp();

        // Change directory is not supported in the current API
        // Just verify we can get current directory
        CHECK(!original_dir.is_empty());
    }
}

TEST_CASE("FileSystem: Directory Iterator")
{
    auto test_dir = create_test_directory(u8"dir_iter");

    // Create test structure
    CHECK(skr::fs::File::write_all_text(test_dir / u8"file1.txt", u8"content"));
    CHECK(skr::fs::File::write_all_text(test_dir / u8"file2.log", u8"content"));
    CHECK(skr::fs::File::write_all_text(test_dir / u8".hidden", u8"content"));
    CHECK(skr::fs::Directory::create(test_dir / u8"subdir1"));
    CHECK(skr::fs::Directory::create(test_dir / u8"subdir2"));

    SUBCASE("Basic iteration")
    {
        struct EntryInfo
        {
            skr::String name;
            skr::fs::FileType type;
        };
        skr::Vector<EntryInfo> entries;

        for (skr::fs::DirectoryIterator iter(test_dir); !iter.at_end(); ++iter)
        {
            const auto& entry = *iter;
            entries.add({ entry.path.filename().string(), entry.type });
        }

        auto find_entry = [&](const char8_t* name) -> EntryInfo* {
            for (auto& e : entries)
            {
                if (e.name == name) return &e;
            }
            return nullptr;
        };

        CHECK(find_entry(u8"file1.txt") != nullptr);
        CHECK(find_entry(u8"file1.txt")->type == skr::fs::FileType::Regular);
        CHECK(find_entry(u8"file2.log") != nullptr);
        CHECK(find_entry(u8"file2.log")->type == skr::fs::FileType::Regular);
        CHECK(find_entry(u8".hidden") != nullptr);
        CHECK(find_entry(u8".hidden")->type == skr::fs::FileType::Regular);
        CHECK(find_entry(u8"subdir1") != nullptr);
        CHECK(find_entry(u8"subdir1")->type == skr::fs::FileType::Directory);
        CHECK(find_entry(u8"subdir2") != nullptr);
        CHECK(find_entry(u8"subdir2")->type == skr::fs::FileType::Directory);
    }

    SUBCASE("Empty directory")
    {
        auto empty_dir = test_dir / u8"empty";
        CHECK(skr::fs::Directory::create(empty_dir));

        size_t count = 0;
        for (skr::fs::DirectoryIterator iter(empty_dir); !iter.at_end(); ++iter)
        {
            count++;
        }
        CHECK(count == 0);
    }

    SUBCASE("Non-existent directory")
    {
        skr::fs::DirectoryIterator iter(test_dir / u8"non_existent");
        CHECK(iter.at_end());
    }

    cleanup_test_directory(test_dir);
}

TEST_CASE("FileSystem: Symlink Operations")
{
    auto test_dir = create_test_directory(u8"symlink");

    SUBCASE("File symlink")
    {
        auto target_file = test_dir / u8"target.txt";
        auto link_file = test_dir / u8"link.txt";

        CHECK(skr::fs::File::write_all_text(target_file, u8"Target content"));

        // Try to create symlink (may fail on some platforms/permissions)
        if (skr::fs::Symlink::create(target_file, link_file))
        {
            // Check symlink properties
            auto info = skr::fs::File::get_info(link_file);
            CHECK(info.exists());
            CHECK(info.is_symlink());

            // Read through symlink
            skr::String content;
            CHECK(skr::fs::File::read_all_text(link_file, content));
            CHECK(content == u8"Target content");

            // Get symlink target
            auto read_target = skr::fs::Symlink::read(link_file);
            CHECK(read_target == target_file);

            // Remove symlink
            CHECK(skr::fs::Symlink::remove(link_file));
            CHECK(!skr::fs::File::exists(link_file));
            CHECK(skr::fs::File::exists(target_file)); // Target should still exist
        }
    }

    cleanup_test_directory(test_dir);
}

TEST_CASE("FileSystem: Path Utilities")
{
    SUBCASE("Absolute path")
    {
        skr::Path relative(u8"test/path");
        skr::Path absolute = skr::fs::PathUtils::absolute(relative);
        CHECK(absolute.is_absolute());
        CHECK(absolute.string().contains(relative.string()));
    }

    SUBCASE("Relative path")
    {
        skr::Path base(u8"/home/user/project");
        skr::Path target(u8"/home/user/project/src/main.cpp");

        skr::Path rel = skr::fs::PathUtils::relative(target, base);
        CHECK(rel.string() == u8"src/main.cpp");
    }

    SUBCASE("Canonical path")
    {
        auto test_dir = create_test_directory(u8"canonical");
        auto subdir = test_dir / u8"subdir";
        CHECK(skr::fs::Directory::create(subdir));

        // Path with dots
        auto with_dots = test_dir / u8"." / u8"subdir" / u8".." / u8"subdir";
        auto canonical = skr::fs::PathUtils::canonical(with_dots);

        // Canonical should resolve all dots
        CHECK(!canonical.string().contains(u8".."));
        CHECK(!canonical.string().contains(u8"/."));

        cleanup_test_directory(test_dir);
    }
}

TEST_CASE("FileSystem: Unicode Support")
{
    auto test_dir = create_test_directory(u8"unicode");

    SUBCASE("Unicode filenames")
    {
        skr::Vector<skr::String> unicode_names = {
            u8"文件名.txt",   // Chinese
            u8"ファイル.txt", // Japanese
            u8"файл.txt",     // Russian
            u8"파일.txt",     // Korean
            u8"café.txt",     // Accented
            u8"🎨🎭.txt",     // Emojis
        };

        for (const auto& name : unicode_names)
        {
            auto file_path = test_dir / name;

            CHECK(skr::fs::File::write_all_text(file_path, u8"Unicode test"));
            CHECK(skr::fs::File::exists(file_path));

            skr::String content;
            CHECK(skr::fs::File::read_all_text(file_path, content));
            CHECK(content == u8"Unicode test");
        }
    }

    SUBCASE("Unicode content")
    {
        auto test_file = test_dir / u8"unicode_content.txt";

        skr::String unicode_content = u8"Hello 世界! 🌍\n"
                                      u8"Привет мир!\n"
                                      u8"こんにちは世界！\n";

        CHECK(skr::fs::File::write_all_text(test_file, unicode_content));

        skr::String read_content;
        CHECK(skr::fs::File::read_all_text(test_file, read_content));
        CHECK(read_content == unicode_content);
    }

    cleanup_test_directory(test_dir);
}

TEST_CASE("FileSystem: Error Handling")
{
    SUBCASE("Invalid operations")
    {
        // Empty path
        CHECK(!skr::fs::File::exists(skr::Path()));
        CHECK(!skr::fs::Directory::exists(skr::Path()));

        // Invalid path characters (OS dependent)
        skr::Path invalid_path(u8"test\0hidden");
        CHECK(!skr::fs::File::exists(invalid_path));
    }

    SUBCASE("Permission errors")
    {
        // Try to write to read-only location
#ifdef _WIN32
        skr::Path read_only(u8"C:/Windows/System32/test_file.txt");
#else
        skr::Path read_only(u8"/test_file.txt");
#endif

        // Should fail gracefully
        CHECK(!skr::fs::File::write_all_text(read_only, u8"test"));
    }
}

TEST_CASE("FileSystem: FileTime Operations")
{
    auto test_dir = create_test_directory(u8"filetime");
    auto test_file = test_dir / u8"time_test.txt";

    CHECK(skr::fs::File::write_all_text(test_file, u8"test"));

    auto info = skr::fs::File::get_info(test_file);

    SUBCASE("Time conversions")
    {
        // Convert to Unix timestamp and back
        auto unix_time = skr::fs::filetime_to_unix(info.last_write_time);
        CHECK(unix_time > 0);

        auto converted_back = skr::fs::unix_to_filetime(unix_time);
        // Allow small difference due to precision loss
        CHECK(std::abs((int64_t)(converted_back - info.last_write_time)) < 10000000); // 1 second
    }

    SUBCASE("Current time")
    {
        // Get current time by creating a new file
        auto temp_file = test_dir / u8"now.txt";
        CHECK(skr::fs::File::write_all_text(temp_file, u8"now"));
        auto now_info = skr::fs::File::get_info(temp_file);
        CHECK(now_info.last_write_time >= info.last_write_time);
    }

    cleanup_test_directory(test_dir);
}