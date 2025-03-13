namespace SB.Core
{
    public struct LinkResult : IArtifact
    {
        public string TargetFile { get; init; }
        public string PDBFile { get; init; }
        public bool IsRestored { get; init; }
    }

    public interface ILinker
    {
        public Version Version { get; }
        public LinkResult Link(IArgumentDriver Driver);
    }
}
