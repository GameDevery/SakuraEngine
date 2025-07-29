#include "SkrOS/filesystem.hpp"
#include "SkrTestFramework/framework.hpp"
#include <cstring>

namespace skr::fs::test
{

TEST_CASE("filesystem basic operations", "[filesystem]")
{
    // 测试临时目录操作
    auto temp_dir = Directory::temp();
    CHECK(!temp_dir.empty());
    CHECK(Directory::exists(temp_dir));
    
    // 测试当前目录
    auto current_dir = Directory::current();
    CHECK(!current_dir.empty());
    CHECK(Directory::exists(current_dir));
    
    // 测试 home 目录
    auto home_dir = Directory::home();
    CHECK(!home_dir.empty());
    CHECK(Directory::exists(home_dir));
}

TEST_CASE("file operations", "[filesystem]")
{
    auto temp_dir = Directory::temp();
    auto test_file = temp_dir / u8"test_file.txt";
    
    // 清理可能存在的测试文件
    if (File::exists(test_file))
    {
        File::remove(test_file);
    }
    
    // 测试文件不存在
    CHECK(!File::exists(test_file));
    
    // 测试写入文件
    skr::StringView test_content = u8"Hello, SakuraEngine Filesystem!";
    CHECK(File::write_all_text(test_file, test_content));
    
    // 测试文件存在
    CHECK(File::exists(test_file));
    
    // 测试读取文件
    skr::String read_content;
    CHECK(File::read_all_text(test_file, read_content));
    CHECK(read_content == test_content);
    
    // 测试文件大小
    CHECK(File::size(test_file) == test_content.size());
    
    // 测试文件信息
    auto info = File::get_info(test_file);
    CHECK(info.exists());
    CHECK(info.is_regular_file());
    CHECK(info.size == test_content.size());
    
    // 清理测试文件
    CHECK(File::remove(test_file));
    CHECK(!File::exists(test_file));
}

TEST_CASE("file handle operations", "[filesystem]")
{
    auto temp_dir = Directory::temp();
    auto test_file = temp_dir / u8"test_handle.txt";
    
    // 清理可能存在的测试文件
    if (File::exists(test_file))
    {
        File::remove(test_file);
    }
    
    // 测试文件创建和写入
    {
        File file;
        CHECK(file.open(test_file, OpenMode::WriteOnly));
        CHECK(file.is_open());
        
        const char8_t* test_data = u8"Test data for file handle";
        size_t data_size = std::strlen(reinterpret_cast<const char*>(test_data));
        
        size_t written = file.write(test_data, data_size);
        CHECK(written == data_size);
        
        file.close();
        CHECK(!file.is_open());
    }
    
    // 测试文件读取
    {
        File file;
        CHECK(file.open(test_file, OpenMode::ReadOnly));
        CHECK(file.is_open());
        
        char8_t buffer[256];
        size_t bytes_read = file.read(buffer, sizeof(buffer) - 1);
        buffer[bytes_read] = 0;
        
        CHECK(skr::StringView(buffer) == u8"Test data for file handle");
        
        file.close();
    }
    
    // 清理测试文件
    CHECK(File::remove(test_file));
}

TEST_CASE("directory operations", "[filesystem]")
{
    auto temp_dir = Directory::temp();
    auto test_dir = temp_dir / u8"test_directory";
    
    // 清理可能存在的测试目录
    if (Directory::exists(test_dir))
    {
        // 简单删除，不递归
        Directory::remove(test_dir, false);
    }
    
    // 测试目录不存在
    CHECK(!Directory::exists(test_dir));
    
    // 测试创建目录
    CHECK(Directory::create(test_dir));
    CHECK(Directory::exists(test_dir));
    
    // 测试目录信息
    auto info = File::get_info(test_dir);
    CHECK(info.exists());
    CHECK(info.is_directory());
    
    // 在目录中创建测试文件
    auto test_file = test_dir / u8"test.txt";
    CHECK(File::write_all_text(test_file, u8"test content"));
    CHECK(File::exists(test_file));
    
    // 清理测试目录（需要先删除文件）
    CHECK(File::remove(test_file));
    CHECK(Directory::remove(test_dir, false));
    CHECK(!Directory::exists(test_dir));
}

TEST_CASE("directory iteration", "[filesystem]")
{
    auto temp_dir = Directory::temp();
    
    // 创建测试目录结构
    auto test_base = temp_dir / u8"test_iteration";
    if (Directory::exists(test_base))
    {
        // 清理现有目录
        if (File::exists(test_base / u8"file1.txt"))
            File::remove(test_base / u8"file1.txt");
        if (File::exists(test_base / u8"file2.txt"))
            File::remove(test_base / u8"file2.txt");
        if (Directory::exists(test_base / u8"subdir"))
            Directory::remove(test_base / u8"subdir", false);
        Directory::remove(test_base, false);
    }
    
    CHECK(Directory::create(test_base));
    CHECK(Directory::create(test_base / u8"subdir"));
    CHECK(File::write_all_text(test_base / u8"file1.txt", u8"content1"));
    CHECK(File::write_all_text(test_base / u8"file2.txt", u8"content2"));
    
    // 测试目录迭代
    size_t file_count = 0;
    size_t dir_count = 0;
    
    for (DirectoryIterator iter(test_base); !iter.at_end(); ++iter)
    {
        const auto& entry = *iter;
        if (entry.type == FileType::Regular)
            file_count++;
        else if (entry.type == FileType::Directory)
            dir_count++;
    }
    
    CHECK(file_count == 2);  // file1.txt, file2.txt
    CHECK(dir_count == 1);   // subdir
    
    // 清理测试目录
    CHECK(File::remove(test_base / u8"file1.txt"));
    CHECK(File::remove(test_base / u8"file2.txt"));
    CHECK(Directory::remove(test_base / u8"subdir", false));
    CHECK(Directory::remove(test_base, false));
}

TEST_CASE("path utilities", "[filesystem]")
{
    // 测试绝对路径转换
    Path relative_path = u8"test/path";
    Path absolute_path = PathUtils::absolute(relative_path);
    CHECK(absolute_path.is_absolute());
    
    // 测试相对路径计算
    Path base = u8"/base/dir";
    Path target = u8"/base/dir/sub/file.txt";
    Path relative = PathUtils::relative(target, base);
    CHECK(relative.string().contains(u8"sub"));
    CHECK(relative.string().contains(u8"file.txt"));
}

#ifdef __APPLE__
TEST_CASE("apple specific directories", "[filesystem][apple]")
{
    auto app_support = Directory::application_support();
    CHECK(!app_support.empty());
    
    auto caches = Directory::caches();
    CHECK(!caches.empty());
    
    // bundle path 可能在非 app 环境下为空，所以不强制检查
    auto bundle = Directory::bundle_path();
    // 只是测试调用不会崩溃
}
#endif

} // namespace skr::fs::test