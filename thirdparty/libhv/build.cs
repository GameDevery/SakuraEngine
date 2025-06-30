using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class LibHV
{
    static LibHV()
    {
        Engine.AddDoctor<LibHVDoctor>();

        Engine.Target("libhv")
            .TargetType(TargetType.HeaderOnly)
            .Defines(Visibility.Public, "HV_STATICLIB")
            .IncludeDirs(Visibility.Public, "include")
            .Link(Visibility.Public, "hv_static")
            .AppleFramework(Visibility.Public, "Security");
    }
}

public class LibHVDoctor : IDoctor
{
    public bool Check()
    {
        Install.SDK("libhv_1.3.3a").Wait();
        return true;
    }
    public bool Fix() 
    { 
        Log.Fatal("core sdks install failed!");
        return true; 
    }
}