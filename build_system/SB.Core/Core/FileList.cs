using SB.Core;
using Microsoft.Extensions.FileSystemGlobbing;

namespace SB
{
    public abstract class FileList
    {
        public FileList AddFiles(params string[] files)
        {
            var Directory = Target!.Directory!;
            lock (FileOperationsLock)
            {
                foreach (var file in files)
                {
                    if (file.Contains("*"))
                    {
                        if (Path.IsPathFullyQualified(file))
                            Globs.Add(Path.GetRelativePath(Directory, file));
                        else
                            Globs.Add(file);
                    }
                    else if (Path.IsPathFullyQualified(file))
                        Absolutes.Add(file);
                    else
                        Absolutes.Add(Path.Combine(Directory, file));
                }
            }
            return this;
        }

        public FileList RemoveFiles(params string[] files)
        {
            var Directory = Target!.Directory!;
            lock (FileOperationsLock)
            {
                foreach (var file in files)
                {
                    if (file.Contains("*"))
                    {
                        if (Path.IsPathFullyQualified(file))
                            Globs.Remove(Path.GetRelativePath(Directory, file));
                        else
                            Globs.Remove(file);
                    }
                    else if (Path.IsPathFullyQualified(file))
                        Absolutes.Remove(file);
                    else
                        Absolutes.Remove(Path.Combine(Directory, file));
                }
            }
            return this;
        }

        internal void GlobFiles()
        {
            if (Globs.Count != 0)
            {
                var GlobMatcher = new Matcher();
                GlobMatcher.AddIncludePatterns(Globs);
                Absolutes.AddRange(GlobMatcher.GetResultsInFullPath(Target!.Directory!));
            }
        }

        public bool Is<T>() => this is T;

        public Target? Target { get; init; }
        public IReadOnlySet<string> Files => Absolutes;
        private SortedSet<string> Globs = new();
        private SortedSet<string> Absolutes = new();
        private System.Threading.Lock FileOperationsLock = new();
    }
}