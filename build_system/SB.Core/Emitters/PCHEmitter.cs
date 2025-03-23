using SB.Core;

namespace SB
{
    using BS = BuildSystem;

    public class PCHEmitter : TaskEmitter
    {
        public PCHEmitter(IToolchain Toolchain) => this.Toolchain = Toolchain;
        public override bool EnableEmitter(Target Target) => Target.GetAttribute<UsePCHAttribute>() != null || Target.GetAttribute<CreatePCHAttribute>() != null;
        public override bool EmitTargetTask(Target Target) => true;
        public override IArtifact? PerTargetTask(Target Target)
        {
            var UsePCH = Target.GetAttribute<UsePCHAttribute>();
            var CreatePCH = Target.GetAttribute<CreatePCHAttribute>();
            bool Changed = false;
            if (CreatePCH is not null)
            {
                var PCHFile = Path.Combine(Target.GetStorePath(BS.GeneratedSourceStore), $"{CreatePCH.Mode}PCH.h");
                var PCHObjFile = Path.Combine(Target.GetStorePath(BS.ObjsStore), $"{CreatePCH.Mode}PCH.pch");

                Changed |= Depend.OnChanged(Target.Name, PCHFile, "PCHEmitter.CreatePCH", (Depend depend) =>
                {
                    var PCHIncludes = String.Join("\n", CreatePCH.Headers.Select(H => $"#include \"{H}\""));
                    var PCHFileContent = $"""
                        #pragma system_header
                        #ifdef __cplusplus
                        #ifdef _WIN32
                        #include <intrin.h>
                        #endif
                        {PCHIncludes}
                        #endif // __cplusplus
                    """;
                    File.WriteAllText(PCHFile, PCHFileContent);
                    depend.ExternalFiles.Add(PCHFile);
                }, CreatePCH.Headers, null);

                Changed |= Depend.OnChanged(Target.Name, PCHFile, "PCHEmitter.CompilePCH", (Depend depend) =>
                {
                    var SourceDependencies = Path.Combine(Target.GetStorePath(BuildSystem.DepsStore), BuildSystem.GetUniqueTempFileName(PCHFile, Target.Name + this.Name, "source.deps.json"));
                    var CompilerDriver = Toolchain.Compiler.CreateArgumentDriver()
                        .AddArguments(Target.Arguments)
                        .AddArgument("Source", PCHFile)
                        .AddArgument("PCHHeader", PCHFile)
                        .AddArgument("PCHObject", PCHObjFile)
                        .AddArgument("SourceDependencies", SourceDependencies);
                    Toolchain.Compiler.Compile(this, Target, CompilerDriver);
                    depend.ExternalFiles.Add(PCHFile);
                    depend.ExternalFiles.Add(PCHObjFile);
                }, CreatePCH.Headers, null);
            }
            if (UsePCH is not null)
            {

            }
            return new PlainArtifact { IsRestored = !Changed };
        }
        private IToolchain Toolchain;
    }

    public enum PCHMode
    {
        Shared,
        Private
    }

    public class UsePCHAttribute
    {
        public required PCHMode Mode { get; init; }
        public string? WantedSharedPCH { get; init; }
    }

    public class CreatePCHAttribute
    {
        public required PCHMode Mode { get; init; }
        public List<string> Headers { get; init; } = new();
        public List<string> Globs { get; init; } = new();
    }

    public static partial class TargetExtensions
    {
        public static Target UseSharedPCH(this Target @this, string? Wanted = null)
        {
            @this.SetAttribute<UsePCHAttribute>(new UsePCHAttribute { Mode = PCHMode.Shared, WantedSharedPCH = Wanted });
            return @this;
        }

        public static Target UsePrivatePCH(this Target @this, params string[] Headers)
        {
            var CreateAttribute = new CreatePCHAttribute { Mode = PCHMode.Private };
            @this.SetAttribute<UsePCHAttribute>(new UsePCHAttribute { Mode = PCHMode.Private });
            @this.SetAttribute<CreatePCHAttribute>(CreateAttribute);
            foreach (var file in Headers)
            {
                if (file.Contains("*"))
                {
                    if (Path.IsPathFullyQualified(file))
                        CreateAttribute.Globs.Add(Path.GetRelativePath(@this.Directory, file));
                    else
                        CreateAttribute.Globs.Add(file);
                }
                else if (Path.IsPathFullyQualified(file))
                    CreateAttribute.Headers.Add(file);
                else
                    CreateAttribute.Headers.Add(Path.Combine(@this.Directory, file));
            }
            return @this;
        }

        public static Target CreateSharedPCH(this Target @this, params string[] Headers)
        {
            var CreateAttribute = new CreatePCHAttribute { Mode = PCHMode.Shared };
            @this.SetAttribute<CreatePCHAttribute>(CreateAttribute);
            foreach (var file in Headers)
            {
                if (file.Contains("*"))
                {
                    if (Path.IsPathFullyQualified(file))
                        CreateAttribute.Globs.Add(Path.GetRelativePath(@this.Directory, file));
                    else
                        CreateAttribute.Globs.Add(file);
                }
                else if (Path.IsPathFullyQualified(file))
                    CreateAttribute.Headers.Add(file);
                else
                    CreateAttribute.Headers.Add(Path.Combine(@this.Directory, file));
            }
            return @this;
        }
    }
}