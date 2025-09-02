using SB;
using SB.Core;
using Serilog;

[TargetScript]
public static class AnimSamples
{
    static AnimSamples()
    {
        Engine.Program("AnimSamplePlayback")
            .AddCppFiles("playback/*.cpp")
            .Depend(Visibility.Private, "AnimSampleCore");
    }
}
