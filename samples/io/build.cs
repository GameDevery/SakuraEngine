using SB;
using SB.Core;

[TargetScript]
public static class IOSamples
{
    static IOSamples()
    {
        Engine.Program("IOSample_load_gltf")
            .IncludeDirs(Visibility.Private, ".")
            .AddCppFiles("gltf/load_gltf.cpp")
            .Depend(Visibility.Private, "SkrRT")
            .Require("cgltf", new PackageConfig { Version = new Version(1, 13, 0) })
            .Depend(Visibility.Private, "cgltf@cgltf");
    }
}