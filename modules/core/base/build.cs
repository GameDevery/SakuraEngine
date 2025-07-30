using SB;
using SB.Core;

[TargetScript]
public static class SkrBase
{
    static SkrBase()
    {
        var Target = Engine.StaticComponent("SkrBase", "SkrCore")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "rtm", "SkrProfile")
            // .Depend(Visibility.Public, "SkrCompileFlags")
            .IncludeDirs(Visibility.Public, "include")
            .Depend(Visibility.Public, "phmap@phmap")
            .Require("phmap", new PackageConfig { Version = new Version(1, 3, 11) })
            .AddCFiles("src/**/build.*.c")
            .AddCppFiles("src/**/build.*.cpp");
        
        // 添加平台特定的文件系统实现
        if (BuildSystem.TargetOS == OSPlatform.Windows)
        {
            Target.Link(Visibility.Private, "advapi32");
            Target.AddCppFiles("src/SkrOS/windows/filesystem.cpp");
        }
        else if (BuildSystem.TargetOS == OSPlatform.OSX)
        {
            Target.AppleFramework(Visibility.Private, "CoreFoundation");
            Target.AddCppFiles("src/SkrOS/unix/filesystem.cpp");
            Target.AddCppFiles("src/SkrOS/apple/filesystem.cpp");
        }
        else // Linux 和其他 Unix-like 系统
        {
            Target.AddCppFiles("src/SkrOS/unix/filesystem.cpp");
        }
    }
}