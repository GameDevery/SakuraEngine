using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class D3D12Agility
{
    static D3D12Agility()
    {
        if (BuildSystem.TargetOS != OSPlatform.Windows)
            return;
            
        // TODO: WE NEED TO ENABLE SETUP IN PACKAGE SCOPE
        BuildSystem.AddSetup<D3D12AgilitySetup>();

        BuildSystem.Package("D3D12Agility")
            .AddTarget("D3D12Agility", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(1, 616, 1))
                {
                    throw new TaskFatalError("D3D12Agility version mismatch!", "D3D12Agility version mismatch, only v1.616.1 is supported in source.");
                }
                var VersionString = Config.Version.ToString();
                Target.TargetType(TargetType.Static)
                    .IncludeDirs(Visibility.Public, VersionString + "/include")
                    .IncludeDirs(Visibility.Private, VersionString + "/include/d3d12-agility")
                    .IncludeDirs(Visibility.Private, VersionString + "/include/d3d12-agility/d3dx12")
                    .AddCppFiles(VersionString + "/src/d3dx12_property_format_table.cpp")
                    .BeforeBuild((Target @this) =>
                    {
                        foreach (var T in BuildSystem.AllTargets)
                        {
                            if (T.Value.GetTargetType() == TargetType.Executable &&
                                T.Value.Dependencies.Contains(@this.Name))
                            {
                                var EXPORT = Path.Combine(SourceLocation.Directory(), VersionString, "src", "export_vars.cpp");
                                T.Value.AddCppFiles(EXPORT);
                            }
                        }                    
                    });
            });
    }
}

public class D3D12AgilitySetup : ISetup
{
    public void Setup()
    {
        if (BuildSystem.TargetOS == OSPlatform.Windows)
        {
            Task.WaitAll(
                Install.SDK("d3d12-agility-1.616.1")
            );
        }
    }
}