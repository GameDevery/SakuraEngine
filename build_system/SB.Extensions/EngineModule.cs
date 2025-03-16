using SB;
using SB.Core;
using System.Diagnostics;
using System.Runtime.CompilerServices;

namespace SB
{   
    public class ModuleAttribute
    {
        public ModuleAttribute(Target Target, string API)
        {
            this.Target = Target;
            this.API = API;
        }
        public Target Target { get; }
        public string API { get; }
    }

    public partial class Project : BuildSystem
    {
        public static Target Module(string Name, string? API = null, [CallerFilePath] string? Location = null)
        {
            API = API ?? Name.ToUpperSnakeCase();
            var Target = BuildSystem.Target(Name, Location!);
            Target.ApplyEngineModulePresets();
            Target.SetAttribute(new ModuleAttribute(Target, API));
            if (ShippingOneArchive)
            {
                Target.TargetType(TargetType.Static);
                Target.Defines(Visibility.Private, "SHIPPING_ONE_ARCHIVE");
                Target.Defines(Visibility.Public, $"{API}_API=");
            }
            else
            {
                Target.TargetType(TargetType.Dynamic);
                Target.Defines(Visibility.Interface, $"{API}_API=SKR_IMPORT");
                Target.Defines(Visibility.Private, $"{API}_API=SKR_EXPORT");
                Target.Defines(Visibility.Private, $"{API}_SHARED");
            }
            return Target;
        }

        public static Target Program(string Name, string? API = null, [CallerFilePath] string? Location = null)
        {
            API = API ?? Name.ToUpperSnakeCase();
            var Target = BuildSystem.Target(Name, Location!);
            Target.ApplyEngineModulePresets();
            Target.SetAttribute(new ModuleAttribute(Target, API));
            Target.TargetType(TargetType.Executable);
            Target.Defines(Visibility.Private, $"{API}_API=");
            if (ShippingOneArchive)
            {
                Target.Defines(Visibility.Private, "SHIPPING_ONE_ARCHIVE");
            }
            return Target;
        }

        public static Target StaticComponent(string Name, string OwnerName, [CallerFilePath] string? Location = null)
        {
            var Target = BuildSystem.Target(Name, Location!);
            Target.ApplyEngineModulePresets();
            Target.TargetType(TargetType.Static);
            Target.BeforeBuild((Target) =>
            {
                var Owner = BuildSystem.GetTarget(OwnerName);
                var OwnerAttribute = Owner?.GetAttribute<ModuleAttribute>();
                var OwnerIncludes = (Owner?.Arguments["IncludeDirs"]) as ArgumentList<string>;
                
                if (OwnerIncludes is not null && OwnerIncludes.Count > 0)
                {
                    var Includes = Target.GetArgumentUnsafe<ArgumentList<string>>("IncludeDirs");
                    Includes.AddRange(OwnerIncludes);
                }

                var Defines = Target.GetArgumentUnsafe<ArgumentList<string>>("Defines");
                if (ShippingOneArchive)
                    Defines.Add($"{OwnerAttribute?.API}_API=");
                else
                    Defines.Add($"{OwnerAttribute?.API}_API=SKR_IMPORT");
            });
            return Target;
        }

        public static bool ShippingOneArchive = false;
        public static bool UseProfile = true;
    }

    public static class EngineModuleExtensions
    {
        public static Target ApplyEngineModulePresets(this Target @this)
        {
            @this.CppVersion("20");
            @this.Exception(false);
            if (BuildSystem.TargetOS == OSPlatform.Windows)
            {
                @this.Defines(Visibility.Private, "NOMINMAX");
            }
            return @this;
        }
    }
}