using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class LibHV
{
    static LibHV()
    {
        Engine.AddSetup<LibHVSetup>();

        Engine.Target("libhv")
            .TargetType(TargetType.HeaderOnly)
            .Defines(Visibility.Public, "HV_STATICLIB")
            .IncludeDirs(Visibility.Public, "include")
            .Link(Visibility.Public, "hv_static")
            .AppleFramework(Visibility.Public, "Security");
    }
}

public class LibHVSetup : ISetup
{
    public void Setup()
    {
        Install.SDK("libhv_1.3.3a").Wait();
    }
}