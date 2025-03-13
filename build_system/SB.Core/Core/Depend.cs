using System.Collections.Immutable;
using System.Runtime.CompilerServices;
using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations.Schema;
using System.Threading.Tasks;

namespace SB.Core
{
    public class DependContext : DbContext
    {
        public DbSet<Depend> Depends { get; set; }
        public string DbPath { get; }

        public DependContext()
        {
            var folder = Environment.SpecialFolder.LocalApplicationData;
            var path = Environment.GetFolderPath(folder);
            DbPath = System.IO.Path.Join(path, "depend.db");
        }

        // The following configures EF to create a Sqlite database file in the special "local" folder for your platform.
        protected override void OnConfiguring(DbContextOptionsBuilder options) => options.UseSqlite($"Data Source={DbPath}");

        public static async Task Initialize()
        {
            await DependContext.Instance.FindAsync<Depend>("");
            ROQuery = Instance.Depends.AsNoTracking();
        }

        public static DependContext Instance = new();
        public static IQueryable<Depend> ROQuery = Instance.Depends.AsNoTracking();
        public static ReaderWriterLockSlim ReadLock = new ReaderWriterLockSlim();
        public static ReaderWriterLockSlim WriteLock = new ReaderWriterLockSlim();
    }
    
    [PrimaryKey(nameof(TargetName))]
    public class Depend
    {
        [Column]
        internal string TargetName { get; set; } = "Invalid";
        public List<string> InputFiles { get; init; }
        public List<DateTime> InputFileTimes { get; init; }
        public List<string>? InputArgs { get; init; }
        public List<DateTime> ExternalFileTimes { get; internal set; }
        [NotMapped]
        public List<string> ExternalFiles { get; } = new();

        public struct Options
        {
            public bool UseSHA { get; init; }
            public bool Force { get; init; }
        }

        public static bool OnChanged(string TargetName, string FileName, string EmitterName, Action<Depend> func, IEnumerable<string> Files, IEnumerable<string> Args, Options? opt = null)
        {
            Options option = opt ?? new Options { Force = false, UseSHA = false };
            var SortedFiles = Files?.ToList() ?? new(); SortedFiles.Sort();
            var SortedArgs = Args?.ToList() ?? new(); SortedArgs.Sort();

            var NeedRerun = option.Force || !CheckDependency(TargetName, FileName, EmitterName, SortedFiles, SortedArgs);
            if (NeedRerun)
            {
                Depend NewDepend = new Depend
                {
                    TargetName = TargetName + FileName + EmitterName,
                    InputFiles = SortedFiles,
                    InputFileTimes = SortedFiles.Select(x => Directory.GetLastWriteTimeUtc(x)).ToList(),
                    InputArgs = SortedArgs
                };
                func(NewDepend);
                UpdateDependency(NewDepend);
                return true;
            }
            return false;
        }

        private static bool CheckDependency(string TargetName, string FileName, string EmitterName, List<string> SortedFiles, List<string> SortedArgs)
        {
            DependContext.ReadLock.EnterWriteLock();
            var Dep = DependContext.ROQuery.Where(D => D.TargetName == TargetName + FileName + EmitterName).First();
            DependContext.ReadLock.ExitWriteLock();

            if (Dep is not null)
            {
                // check file list change
                if (!SortedFiles.SequenceEqual(Dep.InputFiles))
                    return false;
                // check arg list change
                if (!SortedArgs.SequenceEqual(Dep.InputArgs))
                    return false;
                // check input file mtime change
                for (int i = 0; i < Dep.InputFiles.Count; i++)
                {
                    var InputFile = Dep.InputFiles[i];
                    var LastWriteTime = Dep.InputFileTimes[i];

                    if (!File.Exists(InputFile)) // deleted
                        return false;
                    if (LastWriteTime != Directory.GetLastWriteTimeUtc(InputFile)) // modified
                        return false;
                }
                // check output file mtime change
                for (int i = 0; i < Dep.ExternalFiles.Count; i++)
                {
                    var ExternFile = Dep.ExternalFiles[i];
                    var ExternFileTime = Dep.ExternalFileTimes[i];

                    DateTime LastWriteTime;
                    if (!BuildSystem.CachedFileExists(ExternFile, out LastWriteTime)) // deleted
                        return false;
                    if (ExternFileTime != LastWriteTime) // modified
                        return false;
                }
                return true;
            }
            return false;
        }

        private static void UpdateDependency(Depend NewDepend)
        {
            NewDepend.ExternalFiles.Sort();
            NewDepend.ExternalFileTimes = NewDepend.ExternalFiles.Select(x => Directory.GetLastWriteTimeUtc(x)).ToList();
            DependContext.WriteLock.EnterWriteLock();
            DependContext.Instance.Depends.Add(NewDepend);
            DependContext.WriteLock.ExitWriteLock();
            DependContext.Instance.SaveChanges();
        }
    }
}