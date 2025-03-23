using SB.Core;
using Serilog;

namespace SB
{
    using BS = BuildSystem;

    public class UnityBuildAttribute
    {
        public int BatchCount { get; set; } = 10;
    }

    public class UnityBuildEmitter : TaskEmitter
    {
        public override bool EnableEmitter(Target Target) => Target.GetAttribute<UnityBuildAttribute>() != null;
        public override bool EmitTargetTask(Target Target) => Target.AllFiles.Any(F => F.Is_C_Cpp());
        public override IArtifact? PerTargetTask(Target Target)
        {
            var UnityFileDirectory = Path.Combine(Target.GetStorePath(BS.GeneratedSourceStore), "unity_build");
            Directory.CreateDirectory(UnityFileDirectory);

            var UnityBuildAttribute = Target.GetAttribute<UnityBuildAttribute>()!;
            var CBatches = Target.AllFiles.Where(F => F.EndsWith('c')).Chunk(UnityBuildAttribute.BatchCount);
            var CppBatches = Target.AllFiles.Where(F => F.EndsWith("cpp")).Chunk(UnityBuildAttribute.BatchCount);
            
            bool Changed = false;
            List<string> UnityFiles = new();
            var RunBatch = (IEnumerable<string[]> Batches, string Postfix) => {
                int BatchIndex = 0;
                foreach (var Batch in Batches.ToArray())
                {
                    var UnityFile = Path.Combine(UnityFileDirectory, $"unity_build.{BatchIndex++}.{Postfix}");
                    Changed |= Depend.OnChanged(Target.Name, UnityFile, this.Name, (Depend depend) => {
                        var UnityContent = String.Join("\n", Batch.Select(F => $"#include \"{F}\""));
                        File.WriteAllText(UnityFile, UnityContent);
                        depend.ExternalFiles.Add(UnityFile);
                    }, Batch, null);
                    Target.RemoveFiles(Batch);
                    UnityFiles.Add(UnityFile);
                }
            };

            RunBatch(CBatches, "c");
            RunBatch(CppBatches, "cpp");
            Target.AddFiles(UnityFiles.ToArray());

            return new PlainArtifact { IsRestored = !Changed };
        }
    }

    public static partial class TargetExtensions
    {
        public static Target EnableUnityBuild(this Target @this)
        {
            @this.SetAttribute(new UnityBuildAttribute());
            return @this;
        }
    }
}