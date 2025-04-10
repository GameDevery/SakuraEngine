using SB.Core;
using System.Runtime.CompilerServices;

namespace SB
{   
    using BS = BuildSystem;
    [Flags]
    public enum TargetTags
    {
        None = 0,
        ThirdParty = 1,
        Engine = ThirdParty << 1,
        Render = Engine << 1,
        GUI = Render << 1,
        Tool = GUI << 1,
        DCC = Tool << 1,
        Experimental = DCC << 1,
        V8 = Experimental << 1,
        Lua = V8 << 1,
        Tests = Lua << 1,
        Application = Tests << 1,
    }

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

    public partial class Engine : BuildSystem
    {
        public static Target Module(string Name, string? API = null, [CallerFilePath] string? Location = null, [CallerLineNumber] int LineNumber = 0)
        {
            API = API ?? Name.ToUpperSnakeCase();
            var Target = BS.Target(Name, Location!, LineNumber);
            Target.ApplyEngineModulePresets()
                .EnableCodegen(Path.Combine(Target.Directory, "include"))
                .SetAttribute(new ModuleAttribute(Target, API))
                .UseSharedPCH();
            if (ShippingOneArchive)
            {
                Target.TargetType(TargetType.Objects)
                    .Defines(Visibility.Private, "SHIPPING_ONE_ARCHIVE")
                    .Defines(Visibility.Public, $"{API}_API=");
            }
            else
            {
                Target.TargetType(TargetType.Dynamic)
                    .Defines(Visibility.Interface, $"{API}_API=SKR_IMPORT")
                    .Defines(Visibility.Private, $"{API}_API=SKR_EXPORT")
                    .Defines(Visibility.Private, $"{API}_SHARED");
            }
            return Target;
        }

        public static Target Program(string Name, string? API = null, [CallerFilePath] string? Location = null, [CallerLineNumber] int LineNumber = 0)
        {
            API = API ?? Name.ToUpperSnakeCase();
            var Target = BS.Target(Name, Location!, LineNumber);
            Target.ApplyEngineModulePresets();
            Target.EnableCodegen(Path.Combine(Target.Directory, "include"));
            Target.SetAttribute(new ModuleAttribute(Target, API));
            Target.TargetType(TargetType.Executable);
            Target.Defines(Visibility.Private, $"{API}_API=");
            if (ShippingOneArchive)
            {
                Target.Defines(Visibility.Private, "SHIPPING_ONE_ARCHIVE");
            }
            return Target;
        }

        public static Target StaticComponent(string Name, string OwnerName, [CallerFilePath] string? Location = null, [CallerLineNumber] int LineNumber = 0)
        {
            var Target = BS.Target(Name, Location!, LineNumber);
            Target.ApplyEngineModulePresets();
            Target.TargetType(TargetType.Static);
            Target.AfterLoad((Target) =>
            {
                var Owner = BS.GetTarget(OwnerName);
                var OwnerAttribute = Owner?.GetAttribute<ModuleAttribute>();
                var OwnerIncludes = (Owner?.PublicArguments["IncludeDirs"]) as ArgumentList<string>;
                
                if (OwnerIncludes is not null && OwnerIncludes.Count > 0)
                {
                    Target.IncludeDirs(Visibility.Private, OwnerIncludes.ToArray());
                }
                Target.Defines(Visibility.Private, $"{OwnerAttribute?.API}_API=");
            });
            return Target;
        }

        public static void SetTagsUnderDirectory(string Dir, TargetTags Tags, [CallerFilePath] string? Location = null)
        {
            if (!Path.IsPathFullyQualified(Dir))
            {
                Dir = Path.Combine(Path.GetDirectoryName(Location!)!, Dir);
                Dir = Path.GetFullPath(Dir);
            }

            var Matches = BS.AllTargets.Where(KVP => KVP.Value.Directory.StartsWith(Dir)).Select(KVP => KVP.Value);
            foreach (var Target in Matches)
            {
                Target.Tags(Tags);
            }
        }

        public static bool ShippingOneArchive = false;
        public static bool UseProfile = true;
    }

    public static class EngineModuleExtensions
    {
        public static Target ApplyEngineModulePresets(this Target @this)
        {
            @this.CVersion("11");
            @this.CppVersion("20");
            @this.Exception(false);
            if (BS.TargetOS == OSPlatform.Windows)
            {
                @this.CXFlags(Visibility.Private, "/utf-8");
                
                // MSVC & CRT
                @this.Defines(Visibility.Private, "NOMINMAX", "UNICODE", "_UNICODE", "_WINDOWS")
                    .Defines(Visibility.Private, "_CRT_SECURE_NO_WARNINGS")
                    .Defines(Visibility.Private, "_ENABLE_EXTENDED_ALIGNED_STORAGE")
                    .Defines(Visibility.Private, "_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR");

                // Disable Warnings
                @this.ClangCl_CXFlags(Visibility.Private, "-Wno-microsoft-enum-forward-reference")
                    .ClangCl_CXFlags(Visibility.Private, "-Wno-format-security");
            }
            return @this;
        }

        public static bool IsEngineModule(this Target @this) => @this.GetAttribute<ModuleAttribute>() is not null;

        public static Target EnableCodegen(this Target @this, string Root)
        {
            if (!Path.IsPathFullyQualified(Root))
                Root = Path.Combine(@this.Directory, Root);

            if (@this.GetAttribute<CodegenMetaAttribute>() is CodegenMetaAttribute A)
            {
                A.RootDirectory = Root;
                return @this;
            }
            
            @this.SetAttribute(new CodegenMetaAttribute{ RootDirectory = Root });
            @this.SetAttribute(new CodegenRenderAttribute());
            var CodegenDirectory = @this.GetCodegenDirectory();
            Directory.CreateDirectory(CodegenDirectory);
            @this.IncludeDirs(Visibility.Public, CodegenDirectory);
            return @this;
        }

        public static Target AddCodegenScript(this Target @this, string Script, [CallerFilePath] string? Location = null)
        {
            var Scripts = @this.GetAttribute<CodegenRenderAttribute>()!.Scripts;
            if (Path.IsPathFullyQualified(Script))
            {
                Scripts.Add(Script);
                return @this;
            }
            else
            {
                Scripts.Add(Path.Combine(Path.GetDirectoryName(Location!)!, Script));
                return @this;
            }
        }

        public static Target AddSourceGenerators(this Target @this, params string[] Generators)
        {
            var Scripts = @this.GetAttribute<CodegenRenderAttribute>()!.Scripts;
            Scripts.AddRange(Generators);
            return @this;
        }

        public static Target Tags(this Target @this, TargetTags Tags)
        {
            TargetTags Existed = @this.GetAttribute<TargetTags>();
            @this.SetAttribute(Tags | Existed, true);
            return @this;
        }
        public static bool HasTags(this Target @this, TargetTags Tags) => (Tags & @this.GetAttribute<TargetTags>()) == Tags;
    }
}
