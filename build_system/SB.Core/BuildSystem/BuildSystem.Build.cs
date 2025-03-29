using SB.Core;
using Serilog;
using System.Collections.Concurrent;
using System.Diagnostics;
using System.Runtime.CompilerServices;

namespace SB
{
    public partial class BuildSystem
    {
        public static Target Target(string Name, [CallerFilePath] string? Location = null)
        {
            if (AllTargets.TryGetValue(Name, out var Existed))
                throw new ArgumentException($"Target with name {Name} already exists! Name should be unique to every target!");

            var NewTarget = new Target(Name, false, Location!);
            if (!AllTargets.TryAdd(Name, NewTarget))
            {
                throw new ArgumentException($"Failed to add target with name {Name}! Are you adding same targets in parallel?");
            }
            return NewTarget;
        }

        public static Package Package(string Name)
        {
            if (AllPackages.TryGetValue(Name, out var _))
                throw new ArgumentException($"Package with name {Name} already exists! Name should be unique to every package!");

            var NewPackage = new Package(Name);
            if (!AllPackages.TryAdd(Name, NewPackage))
            {
                throw new ArgumentException($"Failed to add package with name {Name}! Are you adding same packages in parallel?");
            }
            return NewPackage;
        }

        public static Target? GetTarget(string Name) => AllTargets.TryGetValue(Name, out var Found) ? Found : null;
        public static Package GetPackage(string Name) => AllPackages.TryGetValue(Name, out var Found) ? Found : throw new ArgumentException($"Package with name {Name} not found!");

        public static TaskEmitter AddTaskEmitter(string Name, TaskEmitter Emitter)
        {
            if (TaskEmitters.TryGetValue(Name, out var _))
                throw new ArgumentException($"Emitter with name {Name} already exists! Name should be unique to every emitter!");
            TaskEmitters.Add(Name, Emitter);
            Emitter.SelfName = Name;
            return Emitter;
        }

        public static void RunBuild()
        {
            try
            {
                using (Profiler.BeginZone("RunBuild", color: (uint)Profiler.ColorType.WebPurple))
                {
                    Task.Run(() => RunBuildImpl(), TaskManager.RootCTS.Token).Wait(TaskManager.RootCTS.Token);
                }
            }
            catch (OperationCanceledException)
            {
                TaskManager.ForceQuit();

                bool First = true;
                TaskFatalError? Fatal = null;
                while (TaskManager.FatalErrors.TryDequeue(out Fatal))
                {
                    if (First)
                    {
                        Log.Error("{FatalTidy} Detail:\n{FatalMessage}", Fatal.Tidy, Fatal.Message);
                        First = false;
                    }
                    else
                    {
                        Log.Error("{FatalTidy}", Fatal.Tidy);
                    }
                }
            }
        }

        public static void RunBuildImpl()
        {
            Dictionary<string, Target> PackageTargets = new();
            foreach (var TargetKVP in AllTargets)
                TargetKVP.Value.ResolvePackages(ref PackageTargets);
            AllTargets.AddRange(PackageTargets);

            foreach (var TargetKVP in AllTargets)
                TargetKVP.Value.ResolveDependencies();

            foreach (var TargetKVP in AllTargets)
                TargetKVP.Value.CallAllActions(TargetKVP.Value.AfterLoadActions);

            foreach (var TargetKVP in AllTargets)
                TargetKVP.Value.ResolveArguments();

            ConcurrentDictionary<TaskFingerprint, Task> EmitterTasks = new();
            var SortedTargets = AllTargets.Values.OrderBy(T => T.Dependencies.Count).ToList();

            // Run Checks
            uint FileTaskCount = 0;
            uint AllTaskCount = 0;
            foreach (var Target in SortedTargets)
            {
                foreach (var EmitterKVP in TaskEmitters)
                {
                    if (EmitterKVP.Value.EmitTargetTask(Target))
                        AllTaskCount += 1;

                    foreach (var FileList in Target.FileLists.Where(FL => EmitterKVP.Value.EmitFileTask(Target, FL)))
                    {
                        foreach (var File in FileList.Files)
                        {
                            FileTaskCount += 1;
                            AllTaskCount += 1;
                        }
                    }
                }
            }

            // Run Build
            uint AllTaskCounter = 0;
            uint FileTaskCounter = 0;
            Parallel.ForEachAsync(SortedTargets, 
            new ParallelOptions { MaxDegreeOfParallelism = Environment.ProcessorCount },
            async (Target Target, CancellationToken Cancel) =>
            {
                Target.CallAllActions(Target.BeforeBuildActions);

                // emitters
                foreach (var EmitterKVP in TaskEmitters)
                {
                    var EmitterName = EmitterKVP.Key;
                    var Emitter = EmitterKVP.Value;
                    TaskFingerprint Fingerprint = new TaskFingerprint
                    {
                        TargetName = Target.Name,
                        File = "",
                        TaskName = EmitterName
                    };
                    if (!Emitter.EnableEmitter(Target))
                    {
                        TaskManager.AddCompleted(Fingerprint);
                        continue;
                    }
                    var EmitterTask = TaskManager.Run(Fingerprint, () =>
                    {
                        List<Task<bool>> FileTasks = new();
                        using (Profiler.BeginZone($"Wait | {Target.Name} | {EmitterName}", color: (uint)Profiler.ColorType.Gray))
                        {
                            if (!Emitter.AwaitExternalTargetDependencies(Target).WaitAndGet())
                                return false;
                            if (!Emitter.AwaitPerTargetDependencies(Target).WaitAndGet())
                                return false;
                        }

                        if (Emitter.EmitTargetTask(Target))
                        {
                            var TaskIndex = Interlocked.Increment(ref AllTaskCounter);
                            var Percentage = 100.0f * TaskIndex / AllTaskCount;
                            using (Profiler.BeginZone($"{EmitterName} | {Target.Name}", color: (uint)Profiler.ColorType.Green1))
                            {
                                Stopwatch sw = new();
                                sw.Start();
                                var TargetTaskArtifact = Emitter.PerTargetTask(Target);
                                sw.Stop();

                                Log.Verbose("[{Percentage:00.0}%] {EmitterName} {TargetName}", Percentage, EmitterName, Target.Name);
                                if (TargetTaskArtifact is not null && !TargetTaskArtifact.IsRestored)
                                {
                                    var CostTime = sw.ElapsedMilliseconds;
                                    Log.Information("[{Percentage:00.0}%]: {EmitterName} {TargetName}, cost {CostTime:00.00}s", Percentage, EmitterName, Target.Name, CostTime / 1000.0f);
                                }
                            }
                        }

                        foreach (var FL in Target.FileLists.ToArray().Where(FL => Emitter.EmitFileTask(Target, FL)))
                        {
                            foreach (var File in FL.Files)
                            {
                                using (Profiler.BeginZone($"Wait | {File} | {EmitterName}", color: (uint)Profiler.ColorType.Gray))
                                {
                                    if (!Emitter.AwaitPerFileDependencies(Target, File).WaitAndGet())
                                        return false;
                                }

                                TaskFingerprint FileFingerprint = new TaskFingerprint
                                {
                                    TargetName = Target.Name,
                                    File = File,
                                    TaskName = EmitterName
                                };
                                var FileTask = TaskManager.Run(FileFingerprint, () =>
                                {
                                    var FileTaskIndex = Interlocked.Increment(ref FileTaskCounter);
                                    var TaskIndex = Interlocked.Increment(ref AllTaskCounter);
                                    var Percentage = 100.0f * TaskIndex / AllTaskCount;

                                    using (Profiler.BeginZone($"{EmitterName} | {Target.Name} | {File}", color: (uint)Profiler.ColorType.Yellow1))
                                    {
                                        Stopwatch sw = new();
                                        sw.Start();
                                        var FileTaskArtifact = Emitter.PerFileTask(Target, FL, FL.GetFileOptions(File), File);
                                        sw.Stop();
                                        Log.Verbose("[{Percentage:00.0}%][{FileTaskIndex}/{FileTaskCount}]: {EmitterName} {TargetName}: {FileName}", Percentage, FileTaskIndex, FileTaskCount, EmitterName, Target.Name, File);
                                        if (FileTaskArtifact is not null && !FileTaskArtifact.IsRestored)
                                        {
                                            var CostTime = sw.ElapsedMilliseconds;
                                            Log.Information("[{Percentage:00.0}%][{FileTaskIndex}/{FileTaskCount}]: {EmitterName} {TargetName}: {FileName}, cost {CostTime:00.00}s", 
                                                Percentage, FileTaskIndex, FileTaskCount, EmitterName, Target.Name, File, CostTime / 1000.0f);
                                        }
                                    }
                                    return true;
                                });
                                FileTasks.Add(FileTask);
                            }
                        }
                        return FileTasks.All(FileTask => (FileTask.Result == true));
                    });
                    EmitterTasks.TryAdd(Fingerprint, EmitterTask);
                    await EmitterTask;
                }
            }).Wait();
            TaskManager.WaitAll(EmitterTasks.Values);
        }

        private static Dictionary<string, TaskEmitter> TaskEmitters = new();
        public static Dictionary<string, Target> AllTargets { get; } = new();
        protected static Dictionary<string, Package> AllPackages { get; } = new();
        internal static Dictionary<string, Target> _AllTargets => AllTargets;
        public delegate void TargetDelegate(Target Target);
        public static TargetDelegate TargetDefaultSettings = new(Target => {
            Target.SetAttribute(new CppCompileAttribute());
            Target.SetAttribute(new CppLinkAttribute());
        });
    }

    internal static class TargetTaskExtensions
    {
        public static async Task<bool> AwaitExternalTargetDependencies(this TaskEmitter Emitter, Target Target)
        {
            bool Success = true;
            foreach (var DepTarget in Target.Dependencies)
            {
                // check target is existed
                if (!BuildSystem._AllTargets.TryGetValue(DepTarget, out var _))
                    throw new ArgumentException($"TargetEmitter {Emitter.Name}: Target {Target.Name} dependes on {DepTarget}, but it seems not to exist!");

                foreach (var DepEmitter in Emitter.Dependencies.Where(KVP => KVP.Value.Equals(DependencyModel.ExternalTarget)))
                {
                    TaskFingerprint Fingerprint = new TaskFingerprint
                    {
                        TargetName = DepTarget,
                        File = "",
                        TaskName = DepEmitter.Key
                    };
                    Success &= await TaskManager.AwaitFingerprint(Fingerprint);
                }
            }
            return Success;
        }
        
        public static async Task<bool> AwaitPerTargetDependencies(this TaskEmitter Emitter, Target Target)
        {
            bool Success = true;
            foreach (var Dependency in Emitter.Dependencies.Where(KVP => KVP.Value.Equals(DependencyModel.PerTarget)))
            {
                TaskFingerprint Fingerprint = new TaskFingerprint
                {
                    TargetName = Target.Name,
                    File = "",
                    TaskName = Dependency.Key
                };
                Success &= await TaskManager.AwaitFingerprint(Fingerprint);
            }
            return Success;
        }

        public static async Task<bool> AwaitPerFileDependencies(this TaskEmitter Emitter, Target Target, string File)
        {
            bool Success = true;
            foreach (var Dependency in Emitter.Dependencies.Where(KVP => KVP.Value.Equals(DependencyModel.PerFile)))
            {
                TaskFingerprint Fingerprint = new TaskFingerprint
                {
                    TargetName = Target.Name,
                    File = File,
                    TaskName = Dependency.Key
                };
                Success &= await TaskManager.AwaitFingerprint(Fingerprint);
            }
            return Success;
        }

        public static bool WaitAndGet(this Task<bool> T)
        {
            try
            {
                T.Wait(TaskManager.RootCTS.Token);
                return T.Result;
            }
            catch (OperationCanceledException)
            {
                return false;
            }
        }

        public static void CallAllActions(this Target Target, IList<Action<Target>> Actions)
        {
            foreach (var Action in Actions)
                Action(Target);
        }
    }
}