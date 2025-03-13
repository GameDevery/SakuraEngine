using SB.Core;
using System.Collections.Generic;

namespace SB
{
    public enum DependencyModel
    {
        PerTarget,
        PerFile,
        ExternalTarget
    }

    public abstract class TaskEmitter
    {
        public TaskEmitter AddDependency(string EmitterName, DependencyModel Model)
        {
            Dependencies.Add(new KeyValuePair<string, DependencyModel>(EmitterName, Model));
            return this;
        }

        public virtual bool EmitTargetTask => false;
        public virtual IArtifact PerTargetTask(Target target) => null;

        public virtual bool EmitFileTask => false;
        public virtual bool FileFilter(string File) => false;
        public virtual IArtifact PerFileTask(Target target, string File) => null;

        public string Name => SelfName;

        internal string SelfName = "";
        internal HashSet<KeyValuePair<string, DependencyModel>> Dependencies { get; } = new();
    }
}
