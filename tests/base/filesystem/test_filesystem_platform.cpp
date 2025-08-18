#include "SkrOS/filesystem.hpp"
#include "SkrTestFramework/framework.hpp"
#include <vector>
#include <thread>
#include <atomic>

// Helper function to create a unique test directory
static skr::Path create_test_directory(const char8_t* name)
{
    auto base = skr::fs::Directory::temp() / u8"skr_fs_platform_test";
    skr::fs::Directory::create(base, true);
    
    static std::atomic<uint64_t> counter{0};
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

TEST_CASE("FileSystem: Platform Specific Operations")
{
    auto test_dir = create_test_directory(u8"platform");
    
    SUBCASE("Path separators on Windows")
    {
#ifdef _WIN32
        auto path1 = test_dir / u8"subdir\\file.txt";
        auto path2 = test_dir / u8"subdir/file.txt";
        
        // Both should normalize to the same path
        CHECK(path1.string() == path2.string());
        
        // Backslashes should be converted to forward slashes
        skr::Path backslash_path(u8"C:\\Windows\\System32");
        CHECK(backslash_path.string() == u8"C:/Windows/System32");
#endif
    }
    
    SUBCASE("Case sensitivity")
    {
        auto file_lower = test_dir / u8"testfile.txt";
        auto file_upper = test_dir / u8"TESTFILE.txt";
        
        CHECK(skr::fs::File::write_all_text(file_lower, u8"lowercase"));
        
#ifdef _WIN32
        // Windows is case-insensitive
        CHECK(skr::fs::File::exists(file_upper));
        
        skr::String content;
        CHECK(skr::fs::File::read_all_text(file_upper, content));
        CHECK(content == u8"lowercase");
#else
        // Unix is case-sensitive
        CHECK(!skr::fs::File::exists(file_upper));
        
        // Create the uppercase version
        CHECK(skr::fs::File::write_all_text(file_upper, u8"uppercase"));
        
        skr::String content_lower, content_upper;
        CHECK(skr::fs::File::read_all_text(file_lower, content_lower));
        CHECK(skr::fs::File::read_all_text(file_upper, content_upper));
        CHECK(content_lower == u8"lowercase");
        CHECK(content_upper == u8"uppercase");
#endif
    }
    
    SUBCASE("Drive letters on Windows")
    {
#ifdef _WIN32
        // Test various Windows path formats
        skr::Path drive_path(u8"C:/");
        CHECK(drive_path.is_absolute());
        
        skr::Path unc_path(u8"//server/share");
        CHECK(unc_path.is_absolute());
        
        // Test drive letter normalization
        skr::Path lower_drive(u8"c:/windows");
        skr::Path upper_drive(u8"C:/windows");
        CHECK(lower_drive.normalize().string() == upper_drive.normalize().string());
#endif
    }
    
    cleanup_test_directory(test_dir);
}

TEST_CASE("FileSystem: Large Scale Operations")
{
    auto test_dir = create_test_directory(u8"large_scale");
    
    SUBCASE("Many files in directory")
    {
        const size_t file_count = 100;
        
        // Create many files
        for (size_t i = 0; i < file_count; i++)
        {
            auto filename = skr::format(u8"file_{:04d}.txt", i);
            auto filepath = test_dir / filename;
            CHECK(skr::fs::File::write_all_text(filepath, skr::format(u8"Content of file {}", i)));
        }
        
        // Count files using iterator
        size_t counted = 0;
        for (skr::fs::DirectoryIterator iter(test_dir); !iter.at_end(); ++iter)
        {
            if (iter->type == skr::fs::FileType::Regular)
            {
                counted++;
            }
        }
        CHECK(counted == file_count);
    }
    
    SUBCASE("Deep directory hierarchy")
    {
        skr::Path current = test_dir;
        const size_t depth = 20;
        
        // Create deep hierarchy
        for (size_t i = 0; i < depth; i++)
        {
            current = current / skr::format(u8"level_{}", i);
        }
        
        CHECK(skr::fs::Directory::create(current, true));
        CHECK(skr::fs::Directory::exists(current));
        
        // Write file at deepest level
        auto deep_file = current / u8"deep.txt";
        CHECK(skr::fs::File::write_all_text(deep_file, u8"Deep content"));
        CHECK(skr::fs::File::exists(deep_file));
    }
    
    cleanup_test_directory(test_dir);
}

TEST_CASE("FileSystem: Concurrent Operations")
{
    auto test_dir = create_test_directory(u8"concurrent");
    
    SUBCASE("Concurrent file writes")
    {
        const int num_threads = 4;
        const int files_per_thread = 25;
        std::vector<std::thread> threads;
        std::atomic<int> success_count{0};
        
        for (int t = 0; t < num_threads; t++)
        {
            threads.emplace_back([test_dir, t, files_per_thread, &success_count]() {
                for (int f = 0; f < files_per_thread; f++)
                {
                    auto filename = skr::format(u8"thread_{}_file_{}.txt", t, f);
                    auto filepath = test_dir / filename;
                    auto content = skr::format(u8"Thread {} File {}", t, f);
                    
                    if (skr::fs::File::write_all_text(filepath, content))
                    {
                        success_count++;
                    }
                }
            });
        }
        
        // Wait for all threads
        for (auto& thread : threads)
        {
            thread.join();
        }
        
        CHECK(success_count == num_threads * files_per_thread);
        
        // Verify all files exist
        size_t file_count = 0;
        for (skr::fs::DirectoryIterator iter(test_dir); !iter.at_end(); ++iter)
        {
            if (iter->type == skr::fs::FileType::Regular)
            {
                file_count++;
            }
        }
        CHECK(file_count == num_threads * files_per_thread);
    }
    
    SUBCASE("Concurrent reads")
    {
        auto test_file = test_dir / u8"shared.txt";
        const skr::String content = u8"Shared content for concurrent reading";
        CHECK(skr::fs::File::write_all_text(test_file, content));
        
        const int num_threads = 8;
        std::vector<std::thread> threads;
        std::atomic<int> success_count{0};
        
        for (int t = 0; t < num_threads; t++)
        {
            threads.emplace_back([test_file, content, &success_count]() {
                // Each thread reads the file multiple times
                for (int i = 0; i < 10; i++)
                {
                    skr::String read_content;
                    if (skr::fs::File::read_all_text(test_file, read_content))
                    {
                        if (read_content == content)
                        {
                            success_count++;
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            });
        }
        
        for (auto& thread : threads)
        {
            thread.join();
        }
        
        CHECK(success_count == num_threads * 10);
    }
    
    cleanup_test_directory(test_dir);
}

TEST_CASE("FileSystem: Special Characters and Edge Cases")
{
    auto test_dir = create_test_directory(u8"special_chars");
    
    SUBCASE("Files with spaces and special characters")
    {
        std::vector<skr::String> special_names = {
            u8"file with spaces.txt",
            u8"file-with-dashes.txt",
            u8"file_with_underscores.txt",
            u8"file.multiple.dots.txt",
            u8"file@symbol.txt",
            u8"file#hash.txt",
            u8"file$dollar.txt",
            u8"file%percent.txt",
            u8"file+plus.txt",
            u8"file=equals.txt",
            u8"(parentheses).txt",
            u8"[brackets].txt",
            u8"{braces}.txt",
        };
        
        size_t success_count = 0;
        for (const auto& name : special_names)
        {
            auto file_path = test_dir / name;
            
            // Some characters might be invalid on certain filesystems
            if (skr::fs::File::write_all_text(file_path, u8"Special char test"))
            {
                CHECK(skr::fs::File::exists(file_path));
                
                skr::String content;
                CHECK(skr::fs::File::read_all_text(file_path, content));
                CHECK(content == u8"Special char test");
                success_count++;
            }
        }
        
        // At least some should succeed
        CHECK(success_count > 0);
    }
    
    SUBCASE("Very long filenames")
    {
        // Create a filename near typical limits
        skr::String long_name;
        for (int i = 0; i < 200; i++)
        {
            long_name.append(u8"a");
        }
        long_name.append(u8".txt");
        
        auto long_file = test_dir / long_name;
        
        // This might fail on some systems due to path length limits
        bool created = skr::fs::File::write_all_text(long_file, u8"test");
        if (created)
        {
            CHECK(skr::fs::File::exists(long_file));
            CHECK(skr::fs::File::remove(long_file));
        }
    }
    
    cleanup_test_directory(test_dir);
}

TEST_CASE("FileSystem: Binary File Operations")
{
    auto test_dir = create_test_directory(u8"binary");
    auto test_file = test_dir / u8"binary.dat";
    
    SUBCASE("Binary data patterns")
    {
        // Create test pattern with all byte values
        skr::Vector<uint8_t> pattern;
        pattern.reserve(256 * 256);
        
        for (int i = 0; i < 256; i++)
        {
            for (int j = 0; j < 256; j++)
            {
                pattern.add(static_cast<uint8_t>(i));
            }
        }
        
        CHECK(skr::fs::File::write_all_bytes(test_file, pattern));
        CHECK(skr::fs::File::size(test_file) == pattern.size());
        
        skr::Vector<uint8_t> read_pattern;
        CHECK(skr::fs::File::read_all_bytes(test_file, read_pattern));
        CHECK(read_pattern.size() == pattern.size());
        CHECK(std::memcmp(read_pattern.data(), pattern.data(), pattern.size()) == 0);
    }
    
    SUBCASE("Zero-length files")
    {
        skr::Vector<uint8_t> empty;
        CHECK(skr::fs::File::write_all_bytes(test_file, empty));
        CHECK(skr::fs::File::exists(test_file));
        CHECK(skr::fs::File::size(test_file) == 0);
        
        skr::Vector<uint8_t> read_empty;
        CHECK(skr::fs::File::read_all_bytes(test_file, read_empty));
        CHECK(read_empty.size() == 0);
    }
    
    cleanup_test_directory(test_dir);
}

TEST_CASE("FileSystem: File Handle Edge Cases")
{
    auto test_dir = create_test_directory(u8"handle_edge");
    
    SUBCASE("Multiple handles to same file")
    {
        auto test_file = test_dir / u8"multi_handle.txt";
        CHECK(skr::fs::File::write_all_text(test_file, u8"Initial content"));
        
        // Open multiple read handles
        skr::fs::File read1, read2;
        CHECK(read1.open(test_file, skr::fs::OpenMode::ReadOnly));
        CHECK(read2.open(test_file, skr::fs::OpenMode::ReadOnly));
        
        char8_t buffer1[100] = {0};
        char8_t buffer2[100] = {0};
        
        CHECK(read1.read(buffer1, 7) == 7);
        CHECK(read2.read(buffer2, 7) == 7);
        
        CHECK(std::memcmp(buffer1, u8"Initial", 7) == 0);
        CHECK(std::memcmp(buffer2, u8"Initial", 7) == 0);
        
        read1.close();
        read2.close();
    }
    
    SUBCASE("Seek beyond file size")
    {
        auto test_file = test_dir / u8"seek_beyond.txt";
        CHECK(skr::fs::File::write_all_text(test_file, u8"Short"));
        
        skr::fs::File file;
        CHECK(file.open(test_file, skr::fs::OpenMode::ReadWrite));
        
        // Seek beyond end
        CHECK(file.seek(100, skr::fs::SeekOrigin::Begin));
        CHECK(file.tell() == 100);
        
        // Write at new position
        CHECK(file.write(u8"Data", 4) == 4);
        
        file.close();
        
        // File should have grown
        CHECK(skr::fs::File::size(test_file) >= 104);
    }
    
    cleanup_test_directory(test_dir);
}

#ifdef __APPLE__
TEST_CASE("FileSystem: macOS Specific")
{
    SUBCASE("macOS special directories")
    {
        auto app_support = skr::fs::Directory::application_support();
        CHECK(!app_support.is_empty());
        // Only check existence if not empty (might be unavailable in test environment)
        if (!app_support.is_empty())
        {
            CHECK(skr::fs::Directory::exists(app_support));
        }
        
        auto caches = skr::fs::Directory::caches();
        CHECK(!caches.is_empty());
        if (!caches.is_empty())
        {
            CHECK(skr::fs::Directory::exists(caches));
        }
        
        // Bundle path might be empty in test environment
        auto bundle = skr::fs::Directory::bundle_path();
        // Just check it doesn't crash
    }
}
#endif

TEST_CASE("FileSystem: Path Resolution Edge Cases")
{
    auto test_dir = create_test_directory(u8"path_resolution");
    
    SUBCASE("Dot and dot-dot handling")
    {
        auto subdir = test_dir / u8"subdir";
        CHECK(skr::fs::Directory::create(subdir));
        
        // Test various dot patterns
        auto same_dir = test_dir / u8"." / u8"subdir";
        CHECK(skr::fs::Directory::exists(same_dir));
        
        auto parent_ref = test_dir / u8"subdir" / u8"..";
        CHECK(skr::fs::Directory::exists(parent_ref));
        
        // Multiple dots
        auto complex_path = test_dir / u8"subdir" / u8"." / u8".." / u8"." / u8"subdir";
        CHECK(skr::fs::Directory::exists(complex_path));
    }
    
    SUBCASE("Trailing slashes")
    {
        auto dir_no_slash = test_dir / u8"testdir";
        auto dir_with_slash = test_dir / u8"testdir/";
        
        CHECK(skr::fs::Directory::create(dir_no_slash));
        
        // Both should refer to the same directory
        CHECK(skr::fs::Directory::exists(dir_with_slash));
        
        // File operations with trailing slash
        auto file_path = test_dir / u8"file.txt";
        CHECK(skr::fs::File::write_all_text(file_path, u8"test"));
        
        // File with trailing slash should still work
        auto file_with_slash = test_dir / u8"file.txt/";
        CHECK(skr::fs::File::exists(file_path)); // Original still works
    }
    
    cleanup_test_directory(test_dir);
}