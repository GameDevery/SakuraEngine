using SB.Core;
using Serilog;
using Serilog.Events;
using Serilog.Sinks.SystemConsole.Themes;
using System.Reflection;

namespace SB
{
    using BS = BuildSystem;
    public partial class Engine : BuildSystem
    {
        public static void SetEngineDirectory(string Directory) => EngineDirectory = Directory;

        public static IToolchain Bootstrap(string ProjectRoot)
        {
            using (Profiler.BeginZone("Bootstrap", color: (uint)Profiler.ColorType.WebMaroon))
            {
                SetupLogger();
                IToolchain? Toolchain = null;
                
                if (BS.HostOS == OSPlatform.Windows)
                    Toolchain = VisualStudioDoctor.VisualStudio;
                else if (BS.HostOS == OSPlatform.OSX)
                    Toolchain = new XCode();
                else
                    throw new Exception("Unsupported Platform!");
                
                // 
                Engine.SetEngineDirectory(ProjectRoot);
                BS.TempPath = Directory.CreateDirectory(Path.Combine(ProjectRoot, ".sb", Toolchain.Name)).FullName;
                BS.BuildPath = Directory.CreateDirectory(Path.Combine(ProjectRoot, ".build", Toolchain.Name)).FullName;
                BS.PackageTempPath = Directory.CreateDirectory(Path.Combine(ProjectRoot, ".pkgs/.sb", Toolchain.Name)).FullName;
                BS.PackageBuildPath = Directory.CreateDirectory(Path.Combine(ProjectRoot, ".pkgs/.build", Toolchain.Name)).FullName;
                BS.LoadConfigurations();

                LoadTargets();
                
                DoctorsTask = Engine.RunDoctors();
                return Toolchain!;
            }
        }

        public static new void RunBuild()
        {
            DoctorsTask!.Wait();
            BS.RunBuild();
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
            BS.TargetDefaultSettings += (Target Target) =>
            {
                Target.CppVersion("20")
                    .RTTI(false)
                    .LinkDirs(Visibility.Public, Target.GetBinaryPath());
                if (BS.TargetOS == OSPlatform.Windows)
                {
                    Target.RuntimeLibrary("MD");
                }
            };

            var Types = typeof(Engine).Assembly.GetTypes();
            var Scripts = Types.Where(Type => Type.GetCustomAttribute<TargetScript>() is not null);
            foreach (var Script in Scripts)
            {
                System.Runtime.CompilerServices.RuntimeHelpers.RunClassConstructor(Script.TypeHandle);
            }
        }
        
        public static string EngineDirectory { get; private set; } = Directory.GetCurrentDirectory();
        public static string ToolDirectory => Path.Combine(EngineDirectory, ".sb/tools");
        public static string DownloadDirectory => Path.Combine(EngineDirectory, ".sb/downloads");
        public static bool EnableDebugInfo = true;
        private static Task? DoctorsTask = null;
    }
}
