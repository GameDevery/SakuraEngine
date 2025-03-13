using System.Collections.Immutable;
using System.IO.Compression;
using System.Runtime.CompilerServices;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace SB.Core
{
    public struct Depend
    {
        [JsonInclude]
        internal readonly ImmutableSortedDictionary<string, DateTime>? InputFiles { get; init; }
        [JsonInclude]
        internal readonly List<string>? InputArgs { get; init; }
        [JsonInclude]
        internal ImmutableSortedDictionary<string, DateTime>? ExternalDeps { get; set; }

        [JsonIgnore]
        public readonly List<string> ExternalFiles { get; } = new();


        public struct Options
        {
            public bool UseSHA { get; init; }
            public bool Force { get; init; }
        }

        public Depend() { }

        public static bool OnChanged(string DepFile, Action<Depend> func, IEnumerable<string> Files, IEnumerable<string> Args, Options? opt = null, [CallerFilePath] string CallerLoc = null)
        {
            Options option = opt ?? new Options { Force = false, UseSHA = false };
            if (!Path.IsPathFullyQualified(DepFile))
            {
                var CallerPath = Path.GetDirectoryName(Path.GetFullPath(CallerLoc));
                DepFile = Path.Combine(CallerPath, DepFile);
            }

            var SortedFiles = Files?.ToList() ?? new(); SortedFiles.Sort();
            var SortedArgs = Args?.ToList() ?? new(); SortedArgs.Sort();

            var NeedRerun = option.Force || !CheckDepFile(DepFile, SortedFiles, SortedArgs);
            if (NeedRerun)
            {
                Depend NewDepend = new Depend
                {
                    InputFiles = SortedFiles.Select(x => new KeyValuePair<string, DateTime>(x, Directory.GetLastWriteTimeUtc(x))).ToImmutableSortedDictionary(),
                    InputArgs = SortedArgs
                };
                func(NewDepend);
                UpdateDepFile(NewDepend, DepFile);
                return true;
            }
            return false;
        }

        private static bool CheckDepFile(string DepFile, List<string> SortedFiles, List<string> SortedArgs)
        {
            if (File.Exists(DepFile))
            {
                var Deps = Json.Deserialize<Depend>(File.ReadAllText(DepFile));
                // check file list change
                if (!SortedFiles.SequenceEqual(Deps.InputFiles.Keys))
                    return false;
                // check arg list change
                if (!SortedArgs.SequenceEqual(Deps.InputArgs))
                    return false;
                // check input file mtime change
                foreach (var InputFile in Deps.InputFiles)
                {
                    if (!File.Exists(InputFile.Key)) // deleted
                        return false;
                    if (InputFile.Value != Directory.GetLastWriteTimeUtc(InputFile.Key)) // modified
                        return false;
                }
                // check output file mtime change
                foreach (var ExternFile in Deps.ExternalDeps)
                {
                    DateTime LastWriteTime;
                    if (!BuildSystem.CachedFileExists(ExternFile.Key, out LastWriteTime)) // deleted
                        return false;
                    if (ExternFile.Value != LastWriteTime) // modified
                        return false;
                }
                return true;
            }
            return false;
        }

        private static void UpdateDepFile(Depend NewDepend, string DepFile)
        {
            NewDepend.ExternalFiles.Sort();
            NewDepend.ExternalDeps = NewDepend.ExternalFiles.Select(x => new KeyValuePair<string, DateTime>(x, Directory.GetLastWriteTimeUtc(x))).ToImmutableSortedDictionary();
            // TODO: CloseHandle seems to be very slow
            // Maybe we can make an async text writer service?
            // It will create & write all files for the process
            File.WriteAllText(DepFile, Json.Serialize(NewDepend));
        }
    }
}