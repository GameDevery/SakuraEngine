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
            var CreateSharedPCH = Target.GetAttribute<CreateSharedPCHAttribute>();
            var CreatePrivatePCH = Target.GetAttribute<CreatePrivatePCHAttribute>();
            bool Changed = false;

            var CreatePCH = (CreatePCHAttribute CreatePCH, PCHMode Mode) => {
                var PCHFile = GetPCHFile(Target, Mode);
                var PCHASTFile = GetPCHASTFile(Target, Mode);

                Changed |= Depend.OnChanged(Target.Name, PCHFile, "PCHEmitter.CreatePCH", (Depend depend) =>
                {
                    var PCHIncludes = String.Join("\n", CreatePCH.Headers.Select(H => $"#include \"{H}\""));
                    var PCHFileContent = $"""
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
                        .AddArgument("AsPCHHeader", true)
                        .AddArgument("Object", PCHASTFile)
                        .AddArgument("SourceDependencies", SourceDependencies);
                    Toolchain.Compiler.Compile(this, Target, CompilerDriver, Path.GetDirectoryName(PCHASTFile));
                    depend.ExternalFiles.Add(PCHFile);
                    depend.ExternalFiles.Add(PCHASTFile);
                }, CreatePCH.Headers, null);
            };

            if (CreateSharedPCH is not null)
                CreatePCH(CreateSharedPCH, PCHMode.Shared);
            if (CreatePrivatePCH is not null)
                CreatePCH(CreatePrivatePCH, PCHMode.Private);

            if (UsePCH is not null)
            {
                string PCHProvider = Target.Name;
                if (UsePCH.Mode == PCHMode.Shared)
                {
                    if (UsePCH.WantedSharedPCH is null)
                    {
                        var Providers = Target.Dependencies.Select(D => BS.GetTarget(D)).Where(D => ProvideSharedPCH(D));
                        var Scores = Providers.Select(P => 1 + P.Dependencies.Count(PD => ProvideSharedPCH(BS.GetTarget(PD))));
                        var BestProvider = Providers.ElementAt(Scores.ToList().IndexOf(Scores.Max()));
                        PCHProvider = BestProvider.Name;
                    }
                    else
                        PCHProvider = UsePCH.WantedSharedPCH!;
                }
                var PCHAST = GetPCHASTFile(BS.GetTarget(PCHProvider), UsePCH.Mode);
                Target.UsePCHAST(PCHAST);
            }
            return new PlainArtifact { IsRestored = !Changed };
        }
        // 2025/3/24
        // under clang-cl, these two files must locate in the same directory
        // because the compiler has bugs for windows path
        private string GetPCHFile(Target Target, PCHMode Mode) => Path.Combine(Target.GetStorePath(BS.GeneratedSourceStore), $"{Mode}PCH.h");
        private string GetPCHASTFile(Target Target, PCHMode Mode) => Path.Combine(Target.GetStorePath(BS.GeneratedSourceStore), $"{Mode}PCH.pch");
        private bool ProvideSharedPCH(Target Target)
        {
            return Target.GetAttribute<CreateSharedPCHAttribute>() is not null;
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

    public abstract class CreatePCHAttribute
    {
        public List<string> Headers { get; init; } = new();
        public List<string> Globs { get; init; } = new();
    }

    public class CreateSharedPCHAttribute : CreatePCHAttribute {}
    
    public class CreatePrivatePCHAttribute : CreatePCHAttribute {}

    public static partial class TargetExtensions
    {
        public static Target UseSharedPCH(this Target @this, string? Wanted = null)
        {
            @this.SetAttribute(new UsePCHAttribute { Mode = PCHMode.Shared, WantedSharedPCH = Wanted });
            return @this;
        }

        public static Target UsePrivatePCH(this Target @this, params string[] Headers)
        {
            var CreateAttribute = new CreatePrivatePCHAttribute();
            @this.SetAttribute(new UsePCHAttribute { Mode = PCHMode.Private });
            @this.SetAttribute(CreateAttribute);
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
            var CreateAttribute = new CreateSharedPCHAttribute();
            @this.SetAttribute(CreateAttribute);
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