using SB.Core;
using Serilog;
using Serilog.Events;
using Serilog.Sinks.SystemConsole.Themes;
using System.Reflection;

namespace SB
{
    public static partial class Utilities
    {
        public static IToolchain Bootstrap(string ProjectRoot)
        {
            using (Profiler.BeginZone("Bootstrap", color: (uint)Profiler.ColorType.WebMaroon))
            {
                SetupLogger();
                IToolchain? Toolchain = null;
                if (BuildSystem.HostOS == OSPlatform.Windows)
                    Toolchain = new VisualStudio(2022);
                else if (BuildSystem.HostOS == OSPlatform.OSX)
                    Toolchain = new XCode();
                else
                    throw new Exception("Unsupported Platform!");
                Engine.SetEngineDirectory(ProjectRoot);
                BuildSystem.TempPath = Directory.CreateDirectory(Path.Combine(ProjectRoot, ".sb")).FullName;
                BuildSystem.BuildPath = Directory.CreateDirectory(Path.Combine(ProjectRoot, ".build")).FullName;
                BuildSystem.PackageTempPath = Directory.CreateDirectory(Path.Combine(ProjectRoot, ".pkgs/.sb")).FullName;
                BuildSystem.PackageBuildPath = Directory.CreateDirectory(Path.Combine(ProjectRoot, ".pkgs/.build")).FullName;
                LoadTargets();
                return Toolchain!;
            }
        }

        private static void SetupLogger()
        {
            SystemConsoleTheme ConsoleLogTheme = new SystemConsoleTheme(
               new Dictionary<ConsoleThemeStyle, SystemConsoleThemeStyle>
               {
                   [ConsoleThemeStyle.Text] = new SystemConsoleThemeStyle { Foreground = ConsoleColor.White },
                   [ConsoleThemeStyle.SecondaryText] = new SystemConsoleThemeStyle { Foreground = ConsoleColor.Gray },
                   [ConsoleThemeStyle.TertiaryText] = new SystemConsoleThemeStyle { Foreground = ConsoleColor.DarkGray },
                   [ConsoleThemeStyle.Invalid] = new SystemConsoleThemeStyle { Foreground = ConsoleColor.Yellow },
                   [ConsoleThemeStyle.Null] = new SystemConsoleThemeStyle { Foreground = ConsoleColor.Blue },
                   [ConsoleThemeStyle.Name] = new SystemConsoleThemeStyle { Foreground = ConsoleColor.Gray },
                   [ConsoleThemeStyle.String] = new SystemConsoleThemeStyle { Foreground = ConsoleColor.White },
                   [ConsoleThemeStyle.Number] = new SystemConsoleThemeStyle { Foreground = ConsoleColor.Green },
                   [ConsoleThemeStyle.Boolean] = new SystemConsoleThemeStyle { Foreground = ConsoleColor.Blue },
                   [ConsoleThemeStyle.Scalar] = new SystemConsoleThemeStyle { Foreground = ConsoleColor.Green },

                   [ConsoleThemeStyle.LevelVerbose] = new SystemConsoleThemeStyle { Foreground = ConsoleColor.Gray },
                   [ConsoleThemeStyle.LevelDebug] = new SystemConsoleThemeStyle { Foreground = ConsoleColor.Gray },
                   [ConsoleThemeStyle.LevelInformation] = new SystemConsoleThemeStyle { Foreground = ConsoleColor.White },
                   [ConsoleThemeStyle.LevelWarning] = new SystemConsoleThemeStyle { Foreground = ConsoleColor.Yellow },
                   [ConsoleThemeStyle.LevelError] = new SystemConsoleThemeStyle { Foreground = ConsoleColor.White, Background = ConsoleColor.Red },
                   [ConsoleThemeStyle.LevelFatal] = new SystemConsoleThemeStyle { Foreground = ConsoleColor.White, Background = ConsoleColor.Red },
               });

            Log.Logger = new LoggerConfiguration()
                .MinimumLevel.Information()
                // .Enrich.WithThreadId()
                // .WriteTo.Console(restrictedToMinimumLevel: LogEventLevel.Information, outputTemplate: "{Timestamp:yyyy-MM-dd HH:mm:ss.ffff zzz} {Message:lj}{NewLine}{Exception}")
                .WriteTo.Async(a => a.Logger(l => l
                    .Filter.ByIncludingOnly(e => e.Level == LogEventLevel.Information)
                    .WriteTo.Console(restrictedToMinimumLevel: LogEventLevel.Information, outputTemplate: "{Message:lj}{NewLine}{Exception}", theme: ConsoleLogTheme)
                ))
                .WriteTo.Async(a => a.Logger(l => l
                    .Filter.ByExcluding(e => e.Level == LogEventLevel.Information)
                    .WriteTo.Console(restrictedToMinimumLevel: LogEventLevel.Information, outputTemplate: "{Level:u}: {Message:lj}{NewLine}{Exception}", theme: ConsoleLogTheme)
                ))
                .CreateLogger();
        }

        private static void LoadTargets()
        {
            var Types = typeof(Utilities).Assembly.GetTypes();
            var Scripts = Types.Where(Type => Type.GetCustomAttribute<TargetScript>() is not null);
            foreach (var Script in Scripts)
            {
                System.Runtime.CompilerServices.RuntimeHelpers.RunClassConstructor(Script.TypeHandle);
            }
        }
    }
}
