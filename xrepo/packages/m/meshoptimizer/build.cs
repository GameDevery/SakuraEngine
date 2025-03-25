using SB;
using SB.Core;

[TargetScript]
public static class MeshOptimizer
{
    static MeshOptimizer()
    {
        BuildSystem
            .Package("MeshOptimizer")
            .AddTarget("MeshOptimizer", (Target Target, PackageConfig Config) =>
            {
                Target
                    .TargetType(TargetType.Static)
                    .CppVersion("20")
                    .AddCppFiles("port/src/*.cpp");
            });
    }
}