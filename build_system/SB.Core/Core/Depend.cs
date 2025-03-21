using System.Collections.Immutable;
using System.Runtime.CompilerServices;
using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations.Schema;
using System.Threading.Tasks;
using Microsoft.EntityFrameworkCore.Infrastructure;
using System.Text.Json.Serialization;

namespace SB.Core
{
    public class DependContext : DbContext
    {
        internal DbSet<DependEntity> Depends { get; set; }

        public DependContext(DbContextOptions<DependContext> options)
            : base(options)
        {

        }

        // The following configures EF to create a Sqlite database file in the special "local" folder for your platform.
        public static async Task Initialize()
        {
            using (var WrapUpCtx = await Factory.CreateDbContextAsync())
            {
                await WrapUpCtx.Database.EnsureCreatedAsync();
                await WrapUpCtx.FindAsync<DependEntity>("");
            }
        }

        public static PooledDbContextFactory<DependContext> Factory = new (
            new DbContextOptionsBuilder<DependContext>()
                .UseSqlite($"Data Source={Path.Join(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "depend.db")}")
                .UseQueryTrackingBehavior(QueryTrackingBehavior.NoTracking)
                .Options
        );
    }

    public struct Depend
    {
        private string PrimaryKey { get; set; } = "Invalid";
        private List<string> InputArgs { get; init; } = new();
        private List<string> InputFiles { get; init; } = new();
        private List<DateTime> InputFileTimes { get; init; } = new();
        private List<DateTime> ExternalFileTimes { get; set; } = new();

        public List<string> ExternalFiles { get; set; } = new();

        public struct Options
        {
            public bool UseSHA { get; init; }
            public bool Force { get; init; }
        }

        public Depend() {}

        public static bool OnChanged(string TargetName, string FileName, string EmitterName, Action<Depend> func, IEnumerable<string>? Files, IEnumerable<string>? Args, Options? opt = null)
        {
            Options option = opt ?? new Options { Force = false, UseSHA = false };
            var SortedFiles = Files?.ToList() ?? new(); SortedFiles.Sort();
            var SortedArgs = Args?.ToList() ?? new(); SortedArgs.Sort();

            Depend? OldDepend = null;
            var NeedRerun = option.Force || !CheckDependency(TargetName, FileName, EmitterName, SortedFiles, SortedArgs, out OldDepend);
            if (NeedRerun)
            {
                Depend NewDepend = new Depend
                {
                    PrimaryKey = TargetName + FileName + EmitterName,
                    InputArgs = SortedArgs,
                    InputFiles = SortedFiles,
                    InputFileTimes = SortedFiles.Select(x => Directory.GetLastWriteTimeUtc(x)).ToList()
                };
                func(NewDepend);
                UpdateDependency(NewDepend, OldDepend);
                return true;
            }
            return false;
        }

        public static async Task<bool> OnChanged(string TargetName, string FileName, string EmitterName, Func<Depend, Task> func, IEnumerable<string>? Files, IEnumerable<string>? Args, Options? opt = null)
        {
            Options option = opt ?? new Options { Force = false, UseSHA = false };
            var SortedFiles = Files?.ToList() ?? new(); SortedFiles.Sort();
            var SortedArgs = Args?.ToList() ?? new(); SortedArgs.Sort();

            Depend? OldDepend = null;
            var NeedRerun = option.Force || !CheckDependency(TargetName, FileName, EmitterName, SortedFiles, SortedArgs, out OldDepend);
            if (NeedRerun)
            {
                Depend NewDepend = new Depend
                {
                    PrimaryKey = TargetName + FileName + EmitterName,
                    InputArgs = SortedArgs,
                    InputFiles = SortedFiles,
                    InputFileTimes = SortedFiles.Select(x => Directory.GetLastWriteTimeUtc(x)).ToList()
                };
                await func(NewDepend);
                UpdateDependency(NewDepend, OldDepend);
                return true;
            }
            return false;
        }

        private static bool CheckDependency(string TargetName, string FileName, string EmitterName, List<string> SortedFiles, List<string> SortedArgs, out Depend? OldDepend)
        {
            OldDepend = null;
            using (var DB = DependContext.Factory.CreateDbContext())
            {
                OldDepend = FromEntity(DB.Depends.Find(TargetName + FileName + EmitterName));
            }
            if (OldDepend is not null)
            {
                // check file list change
                if (!SortedFiles.SequenceEqual(OldDepend?.InputFiles!))
                    return false;
                // check arg list change
                if (!SortedArgs.SequenceEqual(OldDepend?.InputArgs!))
                    return false;
                // check input file mtime change
                for (int i = 0; i < OldDepend?.InputFiles.Count; i++)
                {
                    var InputFile = OldDepend?.InputFiles[i];
                    var DepTime = OldDepend?.InputFileTimes[i];

                    if (!File.Exists(InputFile)) // deleted
                        return false;
                    if (DepTime != Directory.GetLastWriteTimeUtc(InputFile)) // modified
                        return false;
                }
                // check output file mtime change
                for (int i = 0; i < OldDepend?.ExternalFiles.Count; i++)
                {
                    var ExternalFile = OldDepend?.ExternalFiles[i];
                    var DepTime = OldDepend?.ExternalFileTimes[i];

                    DateTime LastWriteTime;
                    if (!BuildSystem.CachedFileExists(ExternalFile!, out LastWriteTime)) // deleted
                        return false;
                    if (DepTime != LastWriteTime) // modified
                        return false;
                }
                return true;
            }
            return false;
        }

        private static void UpdateDependency(Depend NewDepend, Depend? OldDepend)
        {
            NewDepend.ExternalFileTimes = NewDepend.ExternalFiles.Select(x => Directory.GetLastWriteTimeUtc(x)).ToList();

            using (var DB = DependContext.Factory.CreateDbContext())
            {
                if (OldDepend is not null)
                    DB.Depends.Update(ToEntity(NewDepend));
                else
                    DB.Depends.Add(ToEntity(NewDepend));
                    
                DB.SaveChanges();
            }
        }

        private static DependEntity ToEntity(Depend depend)
        {
            return new DependEntity
            {
                PrimaryKey = depend.PrimaryKey,
                InputArgs = depend.InputArgs,
                InputFiles = depend.InputFiles,
                InputFileTimes = depend.InputFileTimes,
                ExternalFiles = depend.ExternalFiles,
                ExternalFileTimes = depend.ExternalFileTimes
            };
        }

        private static Depend? FromEntity(DependEntity? entity)
        {
            if (entity is null)
                return null;

            return new Depend
            {
                PrimaryKey = entity.PrimaryKey,
                InputArgs = entity.InputArgs,
                InputFiles = entity.InputFiles,
                InputFileTimes = entity.InputFileTimes,
                ExternalFiles = entity.ExternalFiles,
                ExternalFileTimes = entity.ExternalFileTimes
            };
        }
    }

    [PrimaryKey(nameof(PrimaryKey))]
    internal class DependEntity
    {
        public string PrimaryKey { get; set; } = "Invalid";
        public List<string> InputArgs { get; init; } = new();
        public List<string> InputFiles { get; init; } = new();
        public List<DateTime> InputFileTimes { get; init; } = new();
        public List<string> ExternalFiles { get; init; } = new();
        public List<DateTime> ExternalFileTimes { get; init; } = new();
    }
}