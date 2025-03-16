using SB;
using SB.Core;

[TargetScript]
public static class SkrBase
{
    static SkrBase()
    {
        var Target = Project.StaticComponent("SkrBase", "SkrCore")
            .Depend(Visibility.Public, "SkrProfile")
            // TODO: MIGRATE COMPILE FLAGS FROM XMAKE
            // .Depend(Visibility.Public, "SkrCompileFlags")
            .IncludeDirs(Visibility.Public, Path.Combine(SourceLocation.Directory(), "include"))
            .AddFiles("src/**/build.*.c", "src/**/build.*.cpp");
        
        if (BuildSystem.TargetOS == OSPlatform.Windows)
            Target.Link(Visibility.Private, "Ole32");
        else if (BuildSystem.TargetOS == OSPlatform.OSX)
            Target.Link(Visibility.Private, "CoreFoundation");
    }
}