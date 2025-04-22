using SB;
using SB.Core;
using Serilog;

[TargetScript]
[LibHVDoctor]
public static class LibHV
{
    static LibHV()
    {
        if (BuildSystem.TargetOS != OSPlatform.Windows)
            return;

        Engine.Target("libhv")
            .TargetType(TargetType.HeaderOnly)
            .Defines(Visibility.Public, "HV_STATICLIB")
            .IncludeDirs(Visibility.Public, "include")
            .Link(Visibility.Public, "hv_static");
    }
}

public class LibHVDoctor : DoctorAttribute
{
    public override bool Check()
    {
        if (BuildSystem.TargetOS != OSPlatform.Windows)
            return true;

        Install.SDK("libhv").Wait();
        return true;
    }
    public override bool Fix() 
    { 
        Log.Fatal("core sdks install failed!");
        return true; 
    }
}