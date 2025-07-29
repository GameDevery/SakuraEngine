#include "SkrTestFramework/framework.hpp"
#include "SkrBase/containers/path/path.hpp"

TEST_CASE("Path: Basic Construction and Normalization")
{
    SUBCASE("Default construction")
    {
        skr::Path p;
        CHECK(p.empty());
        CHECK(p.string().is_empty());
        CHECK(!p.is_absolute());
        CHECK(p.is_relative());
    }
    
    SUBCASE("Backslash to forward slash conversion")
    {
        skr::Path p1(u8"foo\\bar");
        CHECK(p1.string() == u8"foo/bar");
        
        skr::Path p2(u8"foo\\\\bar");
        CHECK(p2.string() == u8"foo/bar");
        
        skr::Path p3(u8"C:\\Windows\\System32", skr::Path::Style::Windows);
        CHECK(p3.string() == u8"C:/Windows/System32");
        
        skr::Path p4(u8"mixed\\forward/slash\\path");
        CHECK(p4.string() == u8"mixed/forward/slash/path");
    }
    
    SUBCASE("Multiple slash handling")
    {
        skr::Path p1(u8"foo//bar");
        CHECK(p1.string() == u8"foo/bar");
        
        skr::Path p2(u8"foo///bar");
        CHECK(p2.string() == u8"foo/bar");
        
        skr::Path p3(u8"//foo/bar");  // UNC path start
        CHECK(p3.string() == u8"/foo/bar");
        
        skr::Path p4(u8"foo//bar//");
        CHECK(p4.string() == u8"foo/bar/");
    }
}

TEST_CASE("Path: Platform-specific Paths")
{
    SUBCASE("Windows absolute paths")
    {
        skr::Path p1(u8"C:/", skr::Path::Style::Windows);
        CHECK(p1.is_absolute());
        CHECK(!p1.is_relative());
        
        skr::Path p2(u8"C:/Windows", skr::Path::Style::Windows);
        CHECK(p2.is_absolute());
        
        skr::Path p3(u8"D:", skr::Path::Style::Windows);
        CHECK(!p3.is_absolute()); // Drive letter alone is not absolute
        
        skr::Path p4(u8"C:/Windows/System32", skr::Path::Style::Windows);
        CHECK(p4.is_absolute());
        
        // Mixed case drive letters
        skr::Path p5(u8"c:/windows", skr::Path::Style::Windows);
        CHECK(p5.is_absolute());
        
        skr::Path p6(u8"Z:/test", skr::Path::Style::Windows);
        CHECK(p6.is_absolute());
    }
    
    SUBCASE("Unix absolute paths")
    {
        skr::Path p1(u8"/", skr::Path::Style::Unix);
        CHECK(p1.is_absolute());
        CHECK(!p1.is_relative());
        
        skr::Path p2(u8"/usr/bin", skr::Path::Style::Unix);
        CHECK(p2.is_absolute());
        
        skr::Path p3(u8"/home/user/file.txt", skr::Path::Style::Unix);
        CHECK(p3.is_absolute());
    }
    
    SUBCASE("Universal style (both Unix and Windows)")
    {
        skr::Path p1(u8"C:/Windows", skr::Path::Style::Universal);
        CHECK(p1.is_absolute());
        
        skr::Path p2(u8"/usr/bin", skr::Path::Style::Universal);
        CHECK(p2.is_absolute());
        
        skr::Path p3(u8"relative/path", skr::Path::Style::Universal);
        CHECK(p3.is_relative());
    }
}

TEST_CASE("Path: Component Extraction")
{
    SUBCASE("filename extraction")
    {
        CHECK(skr::Path(u8"/foo/bar.txt").filename().string() == u8"bar.txt");
        CHECK(skr::Path(u8"bar.txt").filename().string() == u8"bar.txt");
        CHECK(skr::Path(u8"/foo/bar/").filename().string() == u8"");
        CHECK(skr::Path(u8"/foo/bar").filename().string() == u8"bar");
        CHECK(skr::Path(u8"/").filename().string() == u8"");
        CHECK(skr::Path(u8"").filename().string() == u8"");
        CHECK(skr::Path(u8".").filename().string() == u8".");
        CHECK(skr::Path(u8"..").filename().string() == u8"..");
        
        // Windows paths
        CHECK(skr::Path(u8"C:/Windows/System32", skr::Path::Style::Windows).filename().string() == u8"System32");
        CHECK(skr::Path(u8"C:/", skr::Path::Style::Windows).filename().string() == u8"");
    }
    
    SUBCASE("basename extraction")
    {
        CHECK(skr::Path(u8"/foo/bar.txt").basename().string() == u8"bar");
        CHECK(skr::Path(u8"bar.txt").basename().string() == u8"bar");
        CHECK(skr::Path(u8"bar").basename().string() == u8"bar");
        CHECK(skr::Path(u8".hidden").basename().string() == u8".hidden");
        CHECK(skr::Path(u8"bar.tar.gz").basename().string() == u8"bar.tar");
        CHECK(skr::Path(u8"").basename().string() == u8"");
        CHECK(skr::Path(u8".").basename().string() == u8".");
        CHECK(skr::Path(u8"..").basename().string() == u8"..");
        CHECK(skr::Path(u8"file.").basename().string() == u8"file");
        CHECK(skr::Path(u8"file..txt").basename().string() == u8"file.");
    }
    
    SUBCASE("extension extraction")
    {
        CHECK(skr::Path(u8"/foo/bar.txt").extension().string() == u8".txt");
        CHECK(skr::Path(u8"/foo/bar.txt").extension(false).string() == u8"txt");
        CHECK(skr::Path(u8"bar.txt").extension().string() == u8".txt");
        CHECK(skr::Path(u8"bar.txt").extension(false).string() == u8"txt");
        CHECK(skr::Path(u8"bar").extension().string() == u8"");
        CHECK(skr::Path(u8"bar").extension(false).string() == u8"");
        CHECK(skr::Path(u8".hidden").extension().string() == u8"");
        CHECK(skr::Path(u8"bar.tar.gz").extension().string() == u8".gz");
        CHECK(skr::Path(u8"bar.tar.gz").extension(false).string() == u8"gz");
        CHECK(skr::Path(u8"file.").extension().string() == u8".");
        CHECK(skr::Path(u8"file.").extension(false).string() == u8"");
        CHECK(skr::Path(u8"file..txt").extension().string() == u8".txt");
    }
    
    SUBCASE("dir (parent) extraction")
    {
        CHECK(skr::Path(u8"/foo/bar.txt").parent_directory().string() == u8"/foo");
        CHECK(skr::Path(u8"/foo/bar/").parent_directory().string() == u8"/foo");
        CHECK(skr::Path(u8"/foo").parent_directory().string() == u8"/");
        CHECK(skr::Path(u8"/").parent_directory().string() == u8"/");
        CHECK(skr::Path(u8"foo").parent_directory().string() == u8"");
        CHECK(skr::Path(u8"").parent_directory().string() == u8"");
        CHECK(skr::Path(u8"foo/bar").parent_directory().string() == u8"foo");
        CHECK(skr::Path(u8"./foo").parent_directory().string() == u8".");
        CHECK(skr::Path(u8"../foo").parent_directory().string() == u8"..");
        
        // Windows paths
        CHECK(skr::Path(u8"C:/Windows/System32", skr::Path::Style::Windows).parent_directory().string() == u8"C:/Windows");
        CHECK(skr::Path(u8"C:/Windows", skr::Path::Style::Windows).parent_directory().string() == u8"C:/");
        
        // Debug print for C:/ parent
        CHECK(skr::Path(u8"C:/", skr::Path::Style::Windows).parent_directory().string() == u8"C:/");
        // CHECK(skr::Path(u8"C:foo", skr::Path::Style::Windows).parent_directory().string() == u8"C:");
    }
}

TEST_CASE("Path: Normalized")
{
    SUBCASE("Remove single dots")
    {
        CHECK(skr::Path(u8"./foo/bar").normalized().string() == u8"foo/bar");
        CHECK(skr::Path(u8"foo/./bar").normalized().string() == u8"foo/bar");
        CHECK(skr::Path(u8"foo/bar/.").normalized().string() == u8"foo/bar");
        CHECK(skr::Path(u8"./foo/./bar/.").normalized().string() == u8"foo/bar");
        CHECK(skr::Path(u8".").normalized().string() == u8".");
        CHECK(skr::Path(u8"././.").normalized().string() == u8".");
        CHECK(skr::Path(u8"/./foo").normalized().string() == u8"/foo");
    }
    
    SUBCASE("Remove double dots")
    {
        CHECK(skr::Path(u8"foo/../bar").normalized().string() == u8"bar");
        CHECK(skr::Path(u8"foo/bar/..").normalized().string() == u8"foo");
        CHECK(skr::Path(u8"foo/bar/../..").normalized().string() == u8".");
        CHECK(skr::Path(u8"foo/bar/../../..").normalized().string() == u8"..");
        CHECK(skr::Path(u8"../foo").normalized().string() == u8"../foo");
        CHECK(skr::Path(u8"../../foo").normalized().string() == u8"../../foo");
        CHECK(skr::Path(u8"foo/../..").normalized().string() == u8"..");
        CHECK(skr::Path(u8"foo/../../bar").normalized().string() == u8"../bar");
    }
    
    SUBCASE("Absolute path normalization")
    {
        CHECK(skr::Path(u8"/foo/../bar").normalized().string() == u8"/bar");
        CHECK(skr::Path(u8"/foo/bar/..").normalized().string() == u8"/foo");
        CHECK(skr::Path(u8"/foo/..").normalized().string() == u8"/");
        CHECK(skr::Path(u8"/..").normalized().string() == u8"/");
        CHECK(skr::Path(u8"/../foo").normalized().string() == u8"/foo");
        CHECK(skr::Path(u8"/foo/bar/../baz").normalized().string() == u8"/foo/baz");
        
        // Windows absolute paths
        CHECK(skr::Path(u8"C:/foo/../bar", skr::Path::Style::Windows).normalized().string() == u8"C:/bar");
        CHECK(skr::Path(u8"C:/foo/..", skr::Path::Style::Windows).normalized().string() == u8"C:/");
        CHECK(skr::Path(u8"C:/..", skr::Path::Style::Windows).normalized().string() == u8"C:/");
        CHECK(skr::Path(u8"C:/../foo", skr::Path::Style::Windows).normalized().string() == u8"C:/foo");
    }
    
    SUBCASE("Complex normalization")
    {
        CHECK(skr::Path(u8"foo/./bar/../baz").normalized().string() == u8"foo/baz");
        CHECK(skr::Path(u8"./foo/../bar/./../baz").normalized().string() == u8"baz");
        CHECK(skr::Path(u8"foo/bar/../../baz/../qux").normalized().string() == u8"qux");
        CHECK(skr::Path(u8"./foo/./bar/../.././baz").normalized().string() == u8"baz");
        CHECK(skr::Path(u8"foo/bar/baz/../../..").normalized().string() == u8".");
        CHECK(skr::Path(u8"foo/bar/baz/../../../..").normalized().string() == u8"..");
    }
    
    SUBCASE("Empty and special cases")
    {
        CHECK(skr::Path(u8"").normalized().string() == u8"");
        CHECK(skr::Path(u8"//foo").normalized().string() == u8"/foo");
        CHECK(skr::Path(u8"foo//bar").normalized().string() == u8"foo/bar");
        CHECK(skr::Path(u8"foo/").normalized().string() == u8"foo");
        CHECK(skr::Path(u8"foo//").normalized().string() == u8"foo");
    }
}

TEST_CASE("Path: Relative To")
{
    SUBCASE("Simple relative paths")
    {
        CHECK(skr::Path(u8"/a/b/c").relative_to(skr::Path(u8"/a/b")).string() == u8"c");
        CHECK(skr::Path(u8"/a/b").relative_to(skr::Path(u8"/a/b/c")).string() == u8"..");
        CHECK(skr::Path(u8"/a/b/c").relative_to(skr::Path(u8"/a/d")).string() == u8"../b/c");
        CHECK(skr::Path(u8"/a/b").relative_to(skr::Path(u8"/a/b")).string() == u8".");
        CHECK(skr::Path(u8"/a/b/c").relative_to(skr::Path(u8"/")).string() == u8"a/b/c");
        CHECK(skr::Path(u8"/").relative_to(skr::Path(u8"/a/b")).string() == u8"../..");
    }
    
    SUBCASE("Relative to relative paths")
    {
        CHECK(skr::Path(u8"a/b/c").relative_to(skr::Path(u8"a/b")).string() == u8"c");
        CHECK(skr::Path(u8"a/b").relative_to(skr::Path(u8"a/b/c")).string() == u8"..");
        CHECK(skr::Path(u8"a/b/c").relative_to(skr::Path(u8"a/d")).string() == u8"../b/c");
        CHECK(skr::Path(u8"a/b").relative_to(skr::Path(u8"a/b")).string() == u8".");
    }
    
    SUBCASE("Mixed absolute and relative")
    {
        CHECK(skr::Path(u8"/a/b").relative_to(skr::Path(u8"a/b")).string() == u8"");
        CHECK(skr::Path(u8"a/b").relative_to(skr::Path(u8"/a/b")).string() == u8"");
    }
    
    SUBCASE("Complex relative paths")
    {
        CHECK(skr::Path(u8"/a/b/c/d").relative_to(skr::Path(u8"/a/x/y")).string() == u8"../../b/c/d");
        CHECK(skr::Path(u8"/a/x/y").relative_to(skr::Path(u8"/a/b/c/d")).string() == u8"../../../x/y");
        CHECK(skr::Path(u8"/a/b/c").relative_to(skr::Path(u8"/x/y/z")).string() == u8"../../../a/b/c");
    }
    
    SUBCASE("Windows paths")
    {
        auto p1 = skr::Path(u8"C:/a/b/c", skr::Path::Style::Windows);
        auto p2 = skr::Path(u8"C:/a/x", skr::Path::Style::Windows);
        CHECK(p1.relative_to(p2).string() == u8"../b/c");
        
        auto p3 = skr::Path(u8"C:/a/b", skr::Path::Style::Windows);
        auto p4 = skr::Path(u8"C:/a/b/c/d", skr::Path::Style::Windows);
        CHECK(p3.relative_to(p4).string() == u8"../..");
    }
    
    SUBCASE("With normalization needed")
    {
        CHECK(skr::Path(u8"/a/./b/../c").relative_to(skr::Path(u8"/a")).string() == u8"c");
        CHECK(skr::Path(u8"/a/b").relative_to(skr::Path(u8"/a/./b")).string() == u8".");
    }
}

TEST_CASE("Path: Append and Join Operations")
{
    SUBCASE("Basic append")
    {
        skr::Path p1(u8"/foo");
        p1 /= skr::Path(u8"bar");
        CHECK(p1.string() == u8"/foo/bar");
        
        skr::Path p2(u8"/foo/");
        p2 /= skr::Path(u8"bar");
        CHECK(p2.string() == u8"/foo/bar");
        
        skr::Path p3;
        p3 /= skr::Path(u8"bar");
        CHECK(p3.string() == u8"bar");
        
        skr::Path p4(u8"foo");
        p4 /= skr::Path(u8"bar");
        CHECK(p4.string() == u8"foo/bar");
    }
    
    SUBCASE("Append absolute path")
    {
        skr::Path p1(u8"/foo");
        p1 /= skr::Path(u8"/bar");
        CHECK(p1.string() == u8"/bar");
        
        skr::Path p2(u8"C:/foo", skr::Path::Style::Windows);
        p2 /= skr::Path(u8"D:/bar", skr::Path::Style::Windows);
        CHECK(p2.string() == u8"D:/bar");
        
        skr::Path p3(u8"foo");
        p3 /= skr::Path(u8"/bar");
        CHECK(p3.string() == u8"/bar");
    }
    
    SUBCASE("Operator /")
    {
        CHECK((skr::Path(u8"/foo") / skr::Path(u8"bar")).string() == u8"/foo/bar");
        CHECK((skr::Path(u8"foo") / skr::Path(u8"bar")).string() == u8"foo/bar");
        CHECK((skr::Path(u8"") / skr::Path(u8"bar")).string() == u8"bar");
        CHECK((skr::Path(u8"foo") / skr::Path(u8"")).string() == u8"foo/");
        CHECK((skr::Path(u8"/") / skr::Path(u8"bar")).string() == u8"/bar");
    }
    
    SUBCASE("Join alias")
    {
        skr::Path p(u8"/foo");
        p.join(skr::Path(u8"bar"));
        CHECK(p.string() == u8"/foo/bar");
    }
    
    SUBCASE("Append with dots")
    {
        CHECK((skr::Path(u8"foo") / skr::Path(u8".")).string() == u8"foo/.");
        CHECK((skr::Path(u8"foo") / skr::Path(u8"..")).string() == u8"foo/..");
        CHECK((skr::Path(u8"foo") / skr::Path(u8"./bar")).string() == u8"foo/./bar");
    }
}

TEST_CASE("Path: Concat Operations")
{
    skr::Path p1(u8"/foo");
    p1 += skr::Path(u8"bar");
    CHECK(p1.string() == u8"/foobar");
    
    skr::Path p2(u8"test");
    p2 += skr::Path(u8".txt");
    CHECK(p2.string() == u8"test.txt");
    
    skr::Path p3(u8"foo");
    p3 += skr::Path(u8"/bar");
    CHECK(p3.string() == u8"foo/bar");
    
    CHECK((skr::Path(u8"foo") + skr::Path(u8"bar")).string() == u8"foobar");
}

TEST_CASE("Path: Path Manipulation")
{
    SUBCASE("set_filename")
    {
        skr::Path p1(u8"/foo/bar.txt");
        p1.set_filename(skr::Path(u8"baz.cpp"));
        CHECK(p1.string() == u8"/foo/baz.cpp");
        
        skr::Path p2(u8"bar.txt");
        p2.set_filename(skr::Path(u8"new.txt"));
        CHECK(p2.string() == u8"new.txt");
        
        skr::Path p3(u8"/foo/");
        p3.set_filename(skr::Path(u8"bar"));
        CHECK(p3.string() == u8"/foo/bar");
        
        skr::Path p4(u8"/");
        p4.set_filename(skr::Path(u8"foo"));
        CHECK(p4.string() == u8"/foo");
    }
    
    SUBCASE("set_extension")
    {
        skr::Path p1(u8"/foo/bar.txt");
        p1.set_extension(skr::Path(u8".cpp"));
        CHECK(p1.string() == u8"/foo/bar.cpp");
        
        skr::Path p2(u8"file");
        p2.set_extension(skr::Path(u8".txt"));
        CHECK(p2.string() == u8"file.txt");
        
        skr::Path p3(u8"file.old");
        p3.set_extension(skr::Path(u8""));
        CHECK(p3.string() == u8"file");
        
        skr::Path p4(u8"file.tar.gz");
        p4.set_extension(skr::Path(u8".bz2"));
        CHECK(p4.string() == u8"file.tar.bz2");
        
        // Without dot
        skr::Path p5(u8"file.txt");
        p5.set_extension(skr::Path(u8"cpp"));
        CHECK(p5.string() == u8"filecpp");
    }
    
    SUBCASE("remove_extension")
    {
        skr::Path p1(u8"/foo/bar.txt");
        p1.remove_extension();
        CHECK(p1.string() == u8"/foo/bar");
        
        skr::Path p2(u8"file.tar.gz");
        p2.remove_extension();
        CHECK(p2.string() == u8"file.tar");
        
        skr::Path p3(u8"file");
        p3.remove_extension();
        CHECK(p3.string() == u8"file");
        
        // Hidden files should not be affected
        skr::Path p4(u8".hidden");
        p4.remove_extension();
        CHECK(p4.string() == u8".hidden");
        
        skr::Path p5(u8".hidden.txt");
        p5.remove_extension();
        CHECK(p5.string() == u8".hidden");
        
        // Special cases
        skr::Path p6(u8".");
        p6.remove_extension();
        CHECK(p6.string() == u8".");
        
        skr::Path p7(u8"..");
        p7.remove_extension();
        CHECK(p7.string() == u8"..");
        
        // Trailing dots
        skr::Path p8(u8"file.");
        p8.remove_extension();
        CHECK(p8.string() == u8"file");
        
        skr::Path p9(u8"file...");
        p9.remove_extension();
        CHECK(p9.string() == u8"file..");
        
        // Path with directory
        skr::Path p10(u8"/path.ext/file.txt");
        p10.remove_extension();
        CHECK(p10.string() == u8"/path.ext/file");
    }
    
    SUBCASE("replace_extension")
    {
        skr::Path p1(u8"/foo/bar.txt");
        p1.replace_extension(skr::Path(u8".cpp"));
        CHECK(p1.string() == u8"/foo/bar.cpp");
        
        skr::Path p2(u8"file.tar.gz");
        p2.replace_extension(skr::Path(u8".bz2"));
        CHECK(p2.string() == u8"file.tar.bz2");
        
        // File without extension
        skr::Path p3(u8"file");
        p3.replace_extension(skr::Path(u8".txt"));
        CHECK(p3.string() == u8"file.txt");
        
        // Remove extension by replacing with empty
        skr::Path p4(u8"file.txt");
        p4.replace_extension(skr::Path(u8""));
        CHECK(p4.string() == u8"file");
        
        // Hidden files
        skr::Path p5(u8".hidden");
        p5.replace_extension(skr::Path(u8".txt"));
        CHECK(p5.string() == u8".hidden.txt");
        
        skr::Path p6(u8".hidden.old");
        p6.replace_extension(skr::Path(u8".new"));
        CHECK(p6.string() == u8".hidden.new");
        
        // With or without dot
        skr::Path p7(u8"file.txt");
        p7.replace_extension(skr::Path(u8"cpp"));  // without dot
        CHECK(p7.string() == u8"filecpp");
        
        skr::Path p8(u8"file.txt");
        p8.replace_extension(skr::Path(u8".cpp")); // with dot
        CHECK(p8.string() == u8"file.cpp");
    }
}

TEST_CASE("Path: Query Operations")
{
    SUBCASE("empty and is_empty")
    {
        CHECK(skr::Path().empty());
        CHECK(skr::Path().is_empty());
        CHECK(skr::Path(u8"").empty());
        CHECK(!skr::Path(u8"foo").empty());
        CHECK(!skr::Path(u8"/").empty());
    }
    
    SUBCASE("has_parent")
    {
        CHECK(skr::Path(u8"/foo/bar").has_parent());
        CHECK(skr::Path(u8"foo/bar").has_parent());
        CHECK(!skr::Path(u8"foo").has_parent());
        CHECK(!skr::Path(u8"/").has_parent());
        CHECK(!skr::Path(u8"").has_parent());
        CHECK(skr::Path(u8"C:/Windows", skr::Path::Style::Windows).has_parent());
        CHECK(!skr::Path(u8"C:/", skr::Path::Style::Windows).has_parent());
    }
    
    SUBCASE("has_filename")
    {
        CHECK(skr::Path(u8"/foo/bar").has_filename());
        CHECK(skr::Path(u8"bar.txt").has_filename());
        CHECK(!skr::Path(u8"/foo/bar/").has_filename());
        CHECK(!skr::Path(u8"/").has_filename());
        CHECK(!skr::Path(u8"").has_filename());
        CHECK(skr::Path(u8".").has_filename());
        CHECK(skr::Path(u8"..").has_filename());
    }
    
    SUBCASE("has_extension")
    {
        CHECK(skr::Path(u8"foo.txt").has_extension());
        CHECK(skr::Path(u8"foo.tar.gz").has_extension());
        CHECK(!skr::Path(u8"foo").has_extension());
        CHECK(!skr::Path(u8".hidden").has_extension());
        CHECK(!skr::Path(u8"foo.").has_extension());
        CHECK(!skr::Path(u8".").has_extension());
        CHECK(!skr::Path(u8"..").has_extension());
        CHECK(skr::Path(u8"file..txt").has_extension());
    }
}

TEST_CASE("Path: Comparison Operations")
{
    skr::Path p1(u8"foo/bar");
    skr::Path p2(u8"foo/bar");
    skr::Path p3(u8"foo/baz");
    skr::Path p4(u8"foo/Bar");  // Case sensitive
    
    CHECK(p1 == p2);
    CHECK(p1 != p3);
    CHECK(p1 != p4);
    CHECK(p1 < p3);
    CHECK(p3 > p1);
    CHECK(p1 <= p2);
    CHECK(p1 >= p2);
    
    // Normalized comparison
    CHECK(skr::Path(u8"foo/bar") == skr::Path(u8"foo//bar"));
    CHECK(skr::Path(u8"foo/bar") == skr::Path(u8"foo\\bar"));
}

TEST_CASE("Path: Native String Conversion")
{
    SUBCASE("Platform-specific conversion")
    {
#ifdef _WIN32
        skr::Path p1(u8"foo/bar/baz", skr::Path::Style::Windows);
        CHECK(p1.native_string() == u8"foo\\bar\\baz");
        
        skr::Path p2(u8"C:/Windows/System32", skr::Path::Style::Windows);
        CHECK(p2.native_string() == u8"C:\\Windows\\System32");
#else
        skr::Path p1(u8"foo/bar/baz", skr::Path::Style::Unix);
        CHECK(p1.native_string() == u8"foo/bar/baz");
        
        skr::Path p2(u8"/usr/local/bin", skr::Path::Style::Unix);
        CHECK(p2.native_string() == u8"/usr/local/bin");
#endif
    }
    
    SUBCASE("Non-native style")
    {
        // Unix style on any platform
        skr::Path p1(u8"foo/bar", skr::Path::Style::Unix);
        CHECK(p1.native_string() == u8"foo/bar");
        
        // Only convert if style matches platform
#ifndef _WIN32
        skr::Path p2(u8"foo/bar", skr::Path::Style::Windows);
        CHECK(p2.native_string() == u8"foo/bar");
#endif
    }
}

TEST_CASE("Path: Additional Edge Cases from Qt and STL")
{
    SUBCASE("Multiple consecutive separators")
    {
        CHECK(skr::Path(u8"/usr//////lib").normalized().string() == u8"/usr/lib");
        CHECK(skr::Path(u8"foo////bar////baz").normalized().string() == u8"foo/bar/baz");
        CHECK(skr::Path(u8"C://Windows///System32", skr::Path::Style::Windows).normalized().string() == u8"C:/Windows/System32");
        CHECK(skr::Path(u8"////foo").normalized().string() == u8"/foo");
    }
    
    SUBCASE("Trailing dots in filenames")
    {
        // On Unix-like systems, trailing dots are valid
        // The extension is everything after the last dot
        CHECK(skr::Path(u8"file.").extension().string() == u8".");
        CHECK(skr::Path(u8"file.").extension(false).string() == u8"");
        CHECK(skr::Path(u8"file.").basename().string() == u8"file");
        
        CHECK(skr::Path(u8"file..").extension().string() == u8".");
        CHECK(skr::Path(u8"file..").extension(false).string() == u8"");
        CHECK(skr::Path(u8"file..").basename().string() == u8"file.");
        
        CHECK(skr::Path(u8"file...").extension().string() == u8".");
        CHECK(skr::Path(u8"file...").extension(false).string() == u8"");
        CHECK(skr::Path(u8"file...").basename().string() == u8"file..");
        
        // Windows would strip these dots, but we preserve them in the path string
        CHECK(skr::Path(u8"file.", skr::Path::Style::Windows).string() == u8"file.");
    }
    
    SUBCASE("Path comparison with non-canonical paths")
    {
        // Non-canonical paths should compare differently
        CHECK(skr::Path(u8"/a/b/../b") != skr::Path(u8"/a/b"));
        CHECK(skr::Path(u8"/a/b/.") != skr::Path(u8"/a/b"));
        CHECK(skr::Path(u8"./foo") != skr::Path(u8"foo"));
    }
    
    SUBCASE("Normalized paths with complex dot-dot sequences")
    {
        CHECK(skr::Path(u8"a/b/c/../../d").normalized().string() == u8"a/d");
        CHECK(skr::Path(u8"../a/b/../c").normalized().string() == u8"../a/c");
        CHECK(skr::Path(u8"a/../../b").normalized().string() == u8"../b");
        CHECK(skr::Path(u8"./a/b/../../c").normalized().string() == u8"c");
        CHECK(skr::Path(u8"a/./b/../c/./d").normalized().string() == u8"a/c/d");
    }
    
    SUBCASE("Absolute paths with dot-dot at root")
    {
        CHECK(skr::Path(u8"/../foo").normalized().string() == u8"/foo");
        CHECK(skr::Path(u8"/../../foo").normalized().string() == u8"/foo");
        CHECK(skr::Path(u8"C:/../foo", skr::Path::Style::Windows).normalized().string() == u8"C:/foo");
        CHECK(skr::Path(u8"C:/../../foo", skr::Path::Style::Windows).normalized().string() == u8"C:/foo");
    }
    
    SUBCASE("Empty components and trailing separators")
    {
        CHECK(skr::Path(u8"foo//").normalized().string() == u8"foo");
        CHECK(skr::Path(u8"foo/./").normalized().string() == u8"foo");
        CHECK(skr::Path(u8"foo/../").normalized().string() == u8".");
        CHECK(skr::Path(u8"/a/b/").parent_directory().string() == u8"/a");
    }
    
    SUBCASE("Relative paths starting with dot-dot")
    {
        CHECK(skr::Path(u8"../").normalized().string() == u8"..");
        CHECK(skr::Path(u8"../../").normalized().string() == u8"../..");
        CHECK(skr::Path(u8"../foo/..").normalized().string() == u8"..");
        CHECK(skr::Path(u8"../../foo/../bar").normalized().string() == u8"../../bar");
    }
    
    SUBCASE("Mixed path styles")
    {
        // Test that Windows-style paths only work with Windows style
        skr::Path win_path(u8"C:/foo", skr::Path::Style::Unix);
        CHECK(!win_path.is_absolute());
        CHECK(!win_path.is_root());
        
        // Test that Unix paths work with Universal style
        skr::Path unix_path(u8"/foo", skr::Path::Style::Universal);
        CHECK(unix_path.is_absolute());
    }
    
    SUBCASE("Very long paths with many components")
    {
        skr::String long_path;
        for (int i = 0; i < 50; ++i)
        {
            if (i > 0) long_path.append(u8"/");
            long_path.append(u8"component");
        }
        
        auto p = skr::Path(long_path);
        CHECK(p.filename().string() == u8"component");
        CHECK(p.parent_directory().filename().string() == u8"component");
        
        // Test with many dot-dots
        long_path.append(u8"/..");
        for (int i = 0; i < 49; ++i)
        {
            long_path.append(u8"/..");
        }
        CHECK(skr::Path(long_path).normalized().string() == u8".");
    }
    
    SUBCASE("Special Windows paths")
    {
        // Drive letter without slash
        CHECK(skr::Path(u8"C:", skr::Path::Style::Windows).is_root());
        CHECK(!skr::Path(u8"C:foo", skr::Path::Style::Windows).is_absolute());
        
        // Different drive letters
        CHECK(skr::Path(u8"D:/", skr::Path::Style::Windows).is_root());
        CHECK(skr::Path(u8"Z:/test", skr::Path::Style::Windows).is_absolute());
    }
    
    SUBCASE("Path decomposition edge cases")
    {
        // Path ending with separator decomposes differently
        auto p1 = skr::Path(u8"/a/");
        CHECK(!p1.has_filename());
        CHECK(p1.parent_directory().string() == u8"/");
        
        // Single component paths
        CHECK(skr::Path(u8"foo").parent_directory().string() == u8"");
        CHECK(skr::Path(u8"/foo").parent_directory().string() == u8"/");
    }
    
    SUBCASE("Extension detection edge cases")
    {
        CHECK(!skr::Path(u8".").has_extension());
        CHECK(!skr::Path(u8"..").has_extension());
        CHECK(!skr::Path(u8".hidden").has_extension());
        CHECK(skr::Path(u8".hidden.txt").has_extension());
        CHECK(skr::Path(u8"file.").extension().string() == u8".");
        CHECK(skr::Path(u8"file..").extension().string() == u8".");
        CHECK(skr::Path(u8"file...").extension().string() == u8".");
        CHECK(skr::Path(u8"file...txt").extension().string() == u8".txt");
    }
    
    SUBCASE("Relative path calculation edge cases")
    {
        // Same path
        CHECK(skr::Path(u8"/a/b").relative_to(skr::Path(u8"/a/b")).string() == u8".");
        
        // One is prefix of another
        CHECK(skr::Path(u8"/a/b/c/d").relative_to(skr::Path(u8"/a")).string() == u8"b/c/d");
        CHECK(skr::Path(u8"/a").relative_to(skr::Path(u8"/a/b/c")).string() == u8"../..");
        
        // No common prefix
        CHECK(skr::Path(u8"/a/b").relative_to(skr::Path(u8"/x/y")).string() == u8"../../a/b");
        
        // With normalization
        CHECK(skr::Path(u8"/a/./b/../c").relative_to(skr::Path(u8"/a")).string() == u8"c");
    }
}

TEST_CASE("Path: Style System Comprehensive Tests")
{
    SUBCASE("Style-specific absolute path detection")
    {
        // Unix style only recognizes Unix absolute paths
        skr::Path unix_abs(u8"/foo/bar", skr::Path::Style::Unix);
        CHECK(unix_abs.is_absolute());
        
        skr::Path unix_win(u8"C:/foo/bar", skr::Path::Style::Unix);
        CHECK(!unix_win.is_absolute());  // C:/ is not absolute in Unix style
        
        // Windows style only recognizes Windows absolute paths
        skr::Path win_abs(u8"C:/foo/bar", skr::Path::Style::Windows);
        CHECK(win_abs.is_absolute());
        
        skr::Path win_unix(u8"/foo/bar", skr::Path::Style::Windows);
        CHECK(!win_unix.is_absolute());  // / is not absolute in Windows style
        
        // Universal style recognizes both
        skr::Path uni_unix(u8"/foo/bar", skr::Path::Style::Universal);
        CHECK(uni_unix.is_absolute());
        
        skr::Path uni_win(u8"C:/foo/bar", skr::Path::Style::Universal);
        CHECK(uni_win.is_absolute());
    }
    
    SUBCASE("Style-specific root detection")
    {
        // Unix roots
        CHECK(skr::Path(u8"/", skr::Path::Style::Unix).is_root());
        CHECK(!skr::Path(u8"/", skr::Path::Style::Windows).is_root());
        CHECK(skr::Path(u8"/", skr::Path::Style::Universal).is_root());
        
        // Windows roots
        CHECK(skr::Path(u8"C:/", skr::Path::Style::Windows).is_root());
        CHECK(!skr::Path(u8"C:/", skr::Path::Style::Unix).is_root());
        CHECK(skr::Path(u8"C:/", skr::Path::Style::Universal).is_root());
        
        CHECK(skr::Path(u8"C:", skr::Path::Style::Windows).is_root());
        CHECK(!skr::Path(u8"C:", skr::Path::Style::Unix).is_root());
        CHECK(skr::Path(u8"C:", skr::Path::Style::Universal).is_root());
    }
    
    SUBCASE("Style inheritance in operations")
    {
        // Constructed paths inherit style from source
        skr::Path win_path(u8"C:/foo", skr::Path::Style::Windows);
        auto parent = win_path.parent_directory();
        CHECK(parent.style() == skr::Path::Style::Windows);
        
        auto filename = win_path.filename();
        CHECK(filename.style() == skr::Path::Style::Windows);
        
        auto normalized = win_path.normalized();
        CHECK(normalized.style() == skr::Path::Style::Windows);
    }
    
    SUBCASE("Cross-style path operations")
    {
        // Appending preserves the base path's style
        skr::Path base(u8"/foo", skr::Path::Style::Unix);
        skr::Path appended(u8"bar", skr::Path::Style::Windows);
        base.append(appended);
        CHECK(base.string() == u8"/foo/bar");
        CHECK(base.style() == skr::Path::Style::Unix);
        
        // Relative path calculation with different styles but same path type
        skr::Path p1(u8"/a/b/c", skr::Path::Style::Unix);
        skr::Path p2(u8"/a/b", skr::Path::Style::Unix);  // Same style for valid comparison
        auto rel = p1.relative_to(p2);
        CHECK(rel.string() == u8"c");
        CHECK(rel.style() == skr::Path::Style::Unix);
        
        // When styles differ and paths are incompatible, return empty
        skr::Path p3(u8"/a/b/c", skr::Path::Style::Unix);
        skr::Path p4(u8"/a/b", skr::Path::Style::Windows);  // Windows style doesn't recognize / as absolute
        auto rel2 = p3.relative_to(p4);
        CHECK(rel2.string() == u8"");  // Can't compute relative path between absolute and relative
    }
    
    SUBCASE("None style behavior")
    {
        skr::Path none_path(u8"foo/bar", skr::Path::Style::None);
        CHECK(!none_path.is_absolute());  // Nothing is absolute with None style
        CHECK(!none_path.is_root());
        
        skr::Path none_unix(u8"/foo", skr::Path::Style::None);
        CHECK(!none_unix.is_absolute());
        
        skr::Path none_win(u8"C:/foo", skr::Path::Style::None);
        CHECK(!none_win.is_absolute());
    }
    
    SUBCASE("Native style behavior")
    {
#ifdef _WIN32
        CHECK(skr::Path::Style::Native == skr::Path::Style::Windows);
        skr::Path native(u8"C:/foo", skr::Path::Style::Native);
        CHECK(native.is_absolute());
#else
        CHECK(skr::Path::Style::Native == skr::Path::Style::Unix);
        skr::Path native(u8"/foo", skr::Path::Style::Native);
        CHECK(native.is_absolute());
#endif
    }
    
    SUBCASE("Style with normalization")
    {
        // Windows paths should handle drive letters correctly
        CHECK(skr::Path(u8"C:/../foo", skr::Path::Style::Windows).normalized().string() == u8"C:/foo");
        CHECK(skr::Path(u8"D:/a/b/../c", skr::Path::Style::Windows).normalized().string() == u8"D:/a/c");
        
        // Unix style shouldn't treat C: as special
        CHECK(skr::Path(u8"C:/../foo", skr::Path::Style::Unix).normalized().string() == u8"foo");
        
        // Universal style should handle both
        CHECK(skr::Path(u8"C:/../foo", skr::Path::Style::Universal).normalized().string() == u8"C:/foo");
        CHECK(skr::Path(u8"/../foo", skr::Path::Style::Universal).normalized().string() == u8"/foo");
    }
    
    SUBCASE("Style with case sensitivity")
    {
        // Note: Our path class doesn't implement case-insensitive comparison
        // but we test that paths preserve case
        skr::Path win_upper(u8"C:/Windows/System32", skr::Path::Style::Windows);
        skr::Path win_lower(u8"c:/windows/system32", skr::Path::Style::Windows);
        CHECK(win_upper.string() == u8"C:/Windows/System32");
        CHECK(win_lower.string() == u8"c:/windows/system32");
        CHECK(win_upper != win_lower);  // Case-sensitive comparison
    }
}

TEST_CASE("Path: Edge Cases and Corner Cases")
{
    SUBCASE("Empty paths")
    {
        skr::Path empty;
        CHECK(empty.empty());
        CHECK(!empty.has_filename());
        CHECK(!empty.has_extension());
        CHECK(!empty.has_parent());
        CHECK(empty.normalized().empty());
        CHECK(empty.parent_directory().empty());
        CHECK(empty.filename().empty());
        CHECK(empty.basename().empty());
        CHECK(empty.extension().empty());
    }
    
    SUBCASE("Root only paths")
    {
        skr::Path unix_root(u8"/");
        CHECK(unix_root.is_absolute());
        CHECK(unix_root.is_root());
        CHECK(!unix_root.has_filename());
        CHECK(!unix_root.has_extension());
        CHECK(!unix_root.has_parent());
        CHECK(unix_root.parent_directory().string() == u8"/");
        CHECK(unix_root.normalized().string() == u8"/");
        
        skr::Path win_root(u8"C:/", skr::Path::Style::Windows);
        CHECK(win_root.is_absolute());
        CHECK(win_root.is_root());
        CHECK(!win_root.has_filename());
        CHECK(!win_root.has_parent());
        CHECK(win_root.parent_directory().string() == u8"C:/");
        
        // Test C: without slash
        skr::Path win_root2(u8"C:", skr::Path::Style::Windows);
        CHECK(win_root2.is_root());
        CHECK(win_root2.parent_directory().string() == u8"C:");
        
        // Test non-root paths
        CHECK(!skr::Path(u8"/foo").is_root());
        CHECK(!skr::Path(u8"C:/foo", skr::Path::Style::Windows).is_root());
        CHECK(!skr::Path(u8"foo").is_root());
    }
    
    SUBCASE("Dots only paths")
    {
        skr::Path dot(u8".");
        CHECK(dot.has_filename());
        CHECK(dot.filename().string() == u8".");
        CHECK(dot.basename().string() == u8".");
        CHECK(!dot.has_extension());
        CHECK(!dot.has_parent());
        CHECK(dot.normalized().string() == u8".");
        
        skr::Path dotdot(u8"..");
        CHECK(dotdot.has_filename());
        CHECK(dotdot.filename().string() == u8"..");
        CHECK(dotdot.basename().string() == u8"..");
        CHECK(!dotdot.has_extension());
        CHECK(!dotdot.has_parent());
        CHECK(dotdot.normalized().string() == u8"..");
    }
    
    SUBCASE("Hidden files (starting with dot)")
    {
        skr::Path hidden(u8".hidden");
        CHECK(hidden.has_filename());
        CHECK(hidden.filename().string() == u8".hidden");
        CHECK(hidden.basename().string() == u8".hidden");
        CHECK(!hidden.has_extension());
        
        skr::Path hidden_ext(u8".hidden.txt");
        CHECK(hidden_ext.has_filename());
        CHECK(hidden_ext.filename().string() == u8".hidden.txt");
        CHECK(hidden_ext.basename().string() == u8".hidden");
        CHECK(hidden_ext.extension().string() == u8".txt");
    }
    
    SUBCASE("Trailing slashes")
    {
        skr::Path p1(u8"foo/bar/");
        CHECK(!p1.has_filename());
        CHECK(p1.parent_directory().string() == u8"foo");
        CHECK(p1.normalized().string() == u8"foo/bar");
        
        skr::Path p2(u8"foo/bar//");
        CHECK(!p2.has_filename());
        CHECK(p2.normalized().string() == u8"foo/bar");
        
        skr::Path p3(u8"foo/bar///");
        CHECK(p3.string() == u8"foo/bar/");
    }
    
    SUBCASE("Special characters in paths")
    {
        skr::Path spaces(u8"foo bar/baz file.txt");
        CHECK(spaces.filename().string() == u8"baz file.txt");
        CHECK(spaces.basename().string() == u8"baz file");
        CHECK(spaces.extension().string() == u8".txt");
        
        skr::Path special(u8"foo@bar/baz#file$.txt");
        CHECK(special.filename().string() == u8"baz#file$.txt");
        CHECK(special.parent_directory().string() == u8"foo@bar");
    }
    
    SUBCASE("Unicode paths")
    {
        skr::Path unicode(u8"文档/图片/测试.png");
        CHECK(unicode.filename().string() == u8"测试.png");
        CHECK(unicode.basename().string() == u8"测试");
        CHECK(unicode.extension().string() == u8".png");
        CHECK(unicode.parent_directory().string() == u8"文档/图片");
        
        skr::Path emoji(u8"📁/📄.txt");
        CHECK(emoji.filename().string() == u8"📄.txt");
        CHECK(emoji.extension().string() == u8".txt");
    }
    
    SUBCASE("Very long paths")
    {
        skr::String long_component;
        for (int i = 0; i < 50; ++i)
        {
            long_component.append(u8"very_long_component_");
        }
        long_component.append(u8"file.txt");
        
        skr::Path long_path(long_component);
        CHECK(long_path.extension().string() == u8".txt");
        CHECK(long_path.has_filename());
        
        // Deep nesting
        skr::String deep_path;
        for (int i = 0; i < 100; ++i)
        {
            deep_path.append(u8"level/");
        }
        deep_path.append(u8"file.txt");
        
        skr::Path deep(deep_path);
        CHECK(deep.filename().string() == u8"file.txt");
    }
    
    SUBCASE("Invalid Windows drive letters")
    {
        skr::Path invalid1(u8"1:/foo", skr::Path::Style::Windows);
        CHECK(!invalid1.is_absolute());
        
        skr::Path invalid2(u8":/foo", skr::Path::Style::Windows);
        CHECK(!invalid2.is_absolute());
        
        skr::Path invalid3(u8"AB:/foo", skr::Path::Style::Windows);
        CHECK(!invalid3.is_absolute());
    }
    
    SUBCASE("Mixed separators before normalization")
    {
        skr::Path mixed(u8"foo\\bar/baz\\qux");
        CHECK(mixed.string() == u8"foo/bar/baz/qux");
        
        skr::Path mixed2(u8"C:\\foo/bar\\baz", skr::Path::Style::Windows);
        CHECK(mixed2.string() == u8"C:/foo/bar/baz");
        CHECK(mixed2.is_absolute());
    }
    
    SUBCASE("Relative path with too many parent references")
    {
        skr::Path p1(u8"foo/../../bar");
        CHECK(p1.normalized().string() == u8"../bar");
        
        skr::Path p2(u8"foo/../../../bar");
        CHECK(p2.normalized().string() == u8"../../bar");
        
        skr::Path p3(u8"../../../");
        CHECK(p3.normalized().string() == u8"../../..");
    }
    
    SUBCASE("Extension edge cases")
    {
        CHECK(skr::Path(u8"file.").extension().string() == u8".");
        CHECK(skr::Path(u8"file.").extension(false).string() == u8"");
        CHECK(skr::Path(u8"file..").extension().string() == u8".");
        CHECK(skr::Path(u8"file...txt").extension().string() == u8".txt");
        CHECK(skr::Path(u8".").extension().string() == u8"");
        CHECK(skr::Path(u8"..").extension().string() == u8"");
    }
}