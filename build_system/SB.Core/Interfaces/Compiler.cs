namespace SB.Core
{
    public struct CompileResult : IArtifact
    {
        public string ObjectFile { get; init; }
        public string PDBFile { get; init; }
        public bool IsRestored { get; init; }
    }

    public interface ICompiler
    {
        public Version Version { get; }
        public string ExecutablePath { get; }
        public CompileResult Compile(string TargetName, string EmitterName, IArgumentDriver Driver);
    }
}
