using SB;
using SB.Core;
using Serilog;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.CompilerServices;

namespace SB
{
    public static partial class BuildSystem
    {
        public static Target Target(string Name, [CallerFilePath] string Location = null)
        {
            if (AllTargets.TryGetValue(Name, out var _))
                throw new ArgumentException($"Target with name {Name} already exists! Name should be unique to every target!");

            var NewTarget = new Target(Name, Location);
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
        public static Package? GetPackage(string Name) => AllPackages.TryGetValue(Name, out var Found) ? Found : null;

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
                Task.Run(() => RunBuildImpl(), TaskManager.RootCTS.Token).Wait(TaskManager.RootCTS.Token);
            }
            catch (OperationCanceledException OCE)
            {
                TaskManager.ForceQuit();

                bool First = true;
                TaskFatalError Fatal = null;
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
                TargetKVP.Value.ResolveArguments();

            Dictionary<TaskFingerprint, Task> EmitterTasks = new(TaskEmitters.Count * AllTargets.Count);
            var SortedTargets = AllTargets.Values.OrderBy(T => T.Dependencies.Count).ToList();

            // Run Checks
            uint FileTaskCount = 0;
            uint AllTaskCount = 0;
            foreach (var Target in SortedTargets)
            {
                foreach (var EmitterKVP in TaskEmitters)
                {
                    if (EmitterKVP.Value.EmitTargetTask)
                        AllTaskCount += 1;

                    foreach (var File in Target.AllFiles)
                    {
                        if (EmitterKVP.Value.EmitFileTask && EmitterKVP.Value.FileFilter(File))
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
            foreach (var Target in SortedTargets)
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
                    var EmitterTask = TaskManager.Run(Fingerprint, () =>
                    {
                        List<Task<bool>> FileTasks = new (Target.AllFiles.Count);
                        if (!Emitter.AwaitExternalTargetDependencies(Target).WaitAndGet())
                            return false;
                        if (!Emitter.AwaitPerTargetDependencies(Target).WaitAndGet())
                            return false;

                        if (Emitter.EmitTargetTask)
                        {
                            var TaskIndex = Interlocked.Increment(ref AllTaskCounter);
                            var Percentage = 100.0f * TaskIndex / AllTaskCount;

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

                        foreach (var File in Target.AllFiles)
                        {
                            if (!Emitter.FileFilter(File))
                                continue;

                            if (!Emitter.AwaitPerFileDependencies(Target, File).WaitAndGet())
                                return false;

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

                                Stopwatch sw = new();
                                sw.Start();
                                var FileTaskArtifact = Emitter.PerFileTask(Target, File);
                                sw.Stop();

                                Log.Verbose("[{Percentage:00.0}%][{FileTaskIndex}/{FileTaskCount}]: {EmitterName} {TargetName}: {FileName}", Percentage, FileTaskIndex, FileTaskCount, EmitterName, Target.Name, File);
                                if (!FileTaskArtifact.IsRestored)
                                {
                                    var CostTime = sw.ElapsedMilliseconds;
                                    Log.Information("[{Percentage:00.0}%][{FileTaskIndex}/{FileTaskCount}]: {EmitterName} {TargetName}: {FileName}, cost {CostTime:00.00}s", 
                                        Percentage, FileTaskIndex, FileTaskCount, EmitterName, Target.Name, File, CostTime / 1000.0f);
                                }
                                return true;
                            });
                            FileTasks.Add(FileTask);
                        }
                        TaskManager.WaitAll(FileTasks);
                        return FileTasks.All(FileTask => (FileTask.Result == true));
                    });
                    EmitterTasks.Add(Fingerprint, EmitterTask);
                }
            }
            TaskManager.WaitAll(EmitterTasks.Values);
        }

        private static async Task<bool> AwaitExternalTargetDependencies(this TaskEmitter Emitter, Target Target)
        {
            bool Success = true;
            foreach (var DepTarget in Target.Dependencies)
            {
                // check target is existed
                if (!AllTargets.TryGetValue(DepTarget, out var _))
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
        
        private static async Task<bool> AwaitPerTargetDependencies(this TaskEmitter Emitter, Target Target)
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
                Success &= TaskManager.AwaitFingerprint(Fingerprint).WaitAndGet();
            }
            return Success;
        }

        private static async Task<bool> AwaitPerFileDependencies(this TaskEmitter Emitter, Target Target, string File)
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

        private static bool WaitAndGet(this Task<bool> T)
        {
            T.Wait();
            return T.Result;
        }

        private static void CallAllActions(this Target Target, IList<Action<Target>> Actions)
        {
            foreach (var Action in Actions)
                Action(Target);
        }

        private static Dictionary<string, TaskEmitter> TaskEmitters = new();
        internal static Dictionary<string, Target> AllTargets { get; } = new();
        internal static Dictionary<string, Package> AllPackages { get; } = new();
    }
}