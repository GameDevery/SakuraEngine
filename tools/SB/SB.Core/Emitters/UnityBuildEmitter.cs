using SB.Core;

namespace SB
{
    using BS = BuildSystem;

    public class UnityBuildAttribute
    {
        public int BatchCount { get; set; } = 32;
        public required bool Cpp;
        public required bool C;
    }

    public class UnityBuildEmitter : TaskEmitter
    {
        public override bool EnableEmitter(Target Target) =>  (Target.HasFilesOf<CFileList>() || Target.HasFilesOf<CppFileList>()) && Target.HasAttribute<UnityBuildAttribute>();
        public override bool EmitTargetTask(Target Target) => true;
        public override IArtifact? PerTargetTask(Target Target)
        {
            var UnityFileDirectory = Path.Combine(Target.GetStorePath(BS.GeneratedSourceStore), "unity_build");
            Directory.CreateDirectory(UnityFileDirectory);

            var UnityBuildAttribute = Target.GetAttribute<UnityBuildAttribute>()!;
            bool Changed = false;
            var RunBatch = (FileList FileList, IEnumerable<string[]> Batches, string Postfix) => {
                List<string> UnityFiles = new(Batches.Count());
                int BatchIndex = 0;
                foreach (var Batch in Batches.ToArray())
                {
                    var UnityFile = Path.Combine(UnityFileDirectory, $"unity_build.{BatchIndex++}.{Postfix}");
                    Changed |= Depend.OnChanged(Target.Name, UnityFile, this.Name, (Depend depend) => {
                        var UnityContent = String.Join("\n", Batch.Select(F => $"#include \"{F}\""));
                        File.WriteAllText(UnityFile, UnityContent);
                        depend.ExternalFiles.Add(UnityFile);
                    }, Batch, null);
                    FileList.RemoveFiles(Batch);
                    UnityFiles.Add(UnityFile);
                }
                return UnityFiles;
            };

            if (UnityBuildAttribute.C && Target.HasFilesOf<CFileList>())
            {
                var CFilesList = Target.FileList<CFileList>();
                var CBatches = CFilesList.Files.Chunk(UnityBuildAttribute.BatchCount);
                var UnityFiles = RunBatch(CFilesList, CBatches, "c");
                CFilesList.AddFiles(UnityFiles.ToArray());
            }
            
            if (UnityBuildAttribute.Cpp && Target.HasFilesOf<CppFileList>())
            {
                var CppFileList = Target.FileList<CppFileList>();
                var CppBatches = CppFileList.Files.Chunk(UnityBuildAttribute.BatchCount);
                var UnityFiles = RunBatch(CppFileList, CppBatches, "cpp");
                CppFileList.AddFiles(UnityFiles.ToArray());
            }
            
            return new PlainArtifact { IsRestored = !Changed };
        }
    }

    public static partial class TargetExtensions
    {
        public static Target EnableUnityBuild(this Target @this, bool Cpp = true, bool C = true)
        {
            @this.SetAttribute(new UnityBuildAttribute { Cpp = Cpp, C = C });
            return @this;
        }
    }
}