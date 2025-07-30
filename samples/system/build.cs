using SB;
using SB.Core;

[TargetScript]
public static class SystemSamples
{
    static SystemSamples()
    {
        Engine.Program("SystemSample_PlainWindow")
            .IncludeDirs(Visibility.Private, ".")
            .AddCppFiles("plain_window/*.cpp")
            .Depend(Visibility.Private, "SkrSystem")
            .Depend(Visibility.Private, "SkrCore");

        Engine.Program("SystemSample_DPIAlignment")
            .IncludeDirs(Visibility.Private, ".")
            .AddCppFiles("dpi_alignment/*.cpp")
            .Depend(Visibility.Private, "SkrSystem")
            .Depend(Visibility.Private, "SkrCore");


    }
}