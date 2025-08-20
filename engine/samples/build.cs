using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class SampleAssets
{
    static SampleAssets()
    {
        Engine.AddSetup<SampleAssetsSetup>();
    }
}

public class SampleAssetsSetup : ISetup
{
    public void Setup()
    {
        Install.SDK("SampleAssets", new Dictionary<string, string> {
            { "./", SourceLocation.Directory() }
        }, false).Wait();
    }
}