using SB.Core;
using Serilog;

namespace SB
{
    using BS = BuildSystem;

    public class CodegenRenderAttribute
    {
        public List<string> Scripts { get; } = new();
    }

    [CodegenDoctor]
    public class CodegenRenderEmitter : TaskEmitter
    {
        public CodegenRenderEmitter(IToolchain Toolchain)
        {
            this.Toolchain = Toolchain;
        }
        public override bool EnableEmitter(Target Target)
        {
            var RenderAttribute = Target.GetAttribute<CodegenRenderAttribute>();
            if (RenderAttribute is null || !Target.AllFiles.Any(F => F.EndsWith(".h") || F.EndsWith(".hpp")))
                return false;
            if (Target.GetTargetType() == TargetType.HeaderOnly)
                return false;
            return true;
        }
        public override bool EmitTargetTask(Target Target) => true;
        public override IArtifact? PerTargetTask(Target Target)
        {
            var ModAttribute = Target.GetAttribute<ModuleAttribute>()!;
            var MetaAttribute = Target.GetAttribute<CodegenMetaAttribute>()!;
            var RenderAttribute = Target.GetAttribute<CodegenRenderAttribute>()!;

            var CodegenDirectory = Target.GetCodegenDirectory();
            var Config = new CodegenRenderConfig { 
                output_dir = CodegenDirectory,
                main_module = new CodegenModuleInfo { 
                    meta_dir = MetaAttribute.MetaDirectory!,
                    api = ModAttribute.API,
                    module_name = Target.Name
                }
            };
            // Collect include modules
            Target.Dependencies.Select(D => GetModAttr(D)).Where(D => D is not null).Select(D => D!).ToList()
                .ForEach(D => {
                    Config.include_modules.Add(new CodegenModuleInfo {
                        meta_dir = D!.Target.GetAttribute<CodegenMetaAttribute>()!.MetaDirectory!,
                        api = D!.API,
                        module_name = D!.Target.Name
                    });
                });
            // Collect generators
            RenderAttribute.Scripts.Select(S => new CodegenGenerator { entry_file = S }).ToList()
                .ForEach(G => Config.generators.Add(G));
            Target.Dependencies.Select(D => GetGenAttr(D)).Where(D => D is not null).SelectMany(D => D!.Scripts).ToList()
                .ForEach(Script => {
                    Config.generators.Add(new CodegenGenerator {
                        entry_file = Script
                    });
                });
            // Collect depend files
            List<string> DependFiles = new();
            DependFiles.AddRange(RenderAttribute.Scripts);
            if (MetaAttribute.AllGeneratedMetaFiles is not null)
                DependFiles.AddRange(MetaAttribute.AllGeneratedMetaFiles);
            // Execute
            bool Changed = Depend.OnChanged(Target.Name, "", this.Name, (Depend depend) => {
                var ParamFile = Path.Combine(CodegenDirectory, $"{Target.Name}_codegen_config.json");
                File.WriteAllText(ParamFile, Json.Serialize(Config));

                CodegenDoctor.Installation!.Wait();
                var EXE = Path.Combine(CodegenDoctor.Installation.Result, BS.HostOS == OSPlatform.Windows ? "bun.exe" : "bun");
                
                var ExitCode = BS.RunProcess(EXE, $"{GenerateScript} {ParamFile}", out var OutputInfo, out var ErrorInfo, null);
                if (ExitCode != 0)
                {
                    throw new TaskFatalError($"Codegen render {Target.Name} failed with fatal error!", $"bun.exe: {ErrorInfo}");
                }
                var AllGeneratedSources = Directory.GetFiles(CodegenDirectory, "*.cpp", SearchOption.AllDirectories);
                var AllGeneratedHeaders = Directory.GetFiles(CodegenDirectory, "*.h", SearchOption.AllDirectories);
                depend.ExternalFiles.AddRange(AllGeneratedSources);
                depend.ExternalFiles.AddRange(AllGeneratedHeaders);
            }, DependFiles, null);
            // Add generated files to target
            Target.AddFilesLocked(Directory.GetFiles(CodegenDirectory, "*.cpp", SearchOption.AllDirectories));
            return new PlainArtifact { IsRestored = !Changed };
        }

        private static CodegenRenderAttribute? GetGenAttr(string TargetName) => BS.GetTarget(TargetName)?.GetAttribute<CodegenRenderAttribute>();
        private static ModuleAttribute? GetModAttr(string TargetName) => BS.GetTarget(TargetName)?.GetAttribute<ModuleAttribute>();

        // TODO: INSTALL DIRECTORY
        private static string GenerateScript = Path.Combine(Engine.EngineDirectory, "tools/meta_codegen_ts/codegen.ts");
        private IToolchain Toolchain { get; }
        public static volatile int Time = 0;
    }

    public struct CodegenModuleInfo
    {
        public string meta_dir { get; set; }
        public string api { get; set; }
        public string module_name { get; set; }
    }

    public class CodegenGenerator
    {
        public required string entry_file { get; set; }
        IReadOnlyList<string> import_dirs { get; set; } = new List<string>();
    }

    public class CodegenRenderConfig
    {
        public List<CodegenModuleInfo> include_modules { get; set;} = new();
        public CodegenModuleInfo main_module { get; set; }
        public required string output_dir { get; set; }
        public List<CodegenGenerator> generators { get; set; } = new();
    }

    public static class TargetExtensions
    {
        public static string GetCodegenDirectory(this Target @this) => Path.Combine(@this.GetStorePath(BuildSystem.GeneratedSourceStore), $"codegen/{@this.Name}");
    }

    public class CodegenDoctor : DoctorAttribute
    {
        public override bool Check()
        {
            Installation = Install.Tool("bun_1.2.5");
            return true;
        }
        public override bool Fix() 
        { 
            Log.Fatal("bun install failed!");
            return true; 
        }
        public static Task<string>? Installation;
    }
}