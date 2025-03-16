using System.Collections.Concurrent;

namespace SB.Core
{
    public struct TaskFingerprint
    {
        public string TargetName { get; init; }
        public string File { get; init; }
        public string TaskName { get; init; }
    }

    public class TaskFatalError : Exception
    {
        public TaskFatalError(string tidy, string what)
            : base(what)
        {
            Tidy = tidy;
        }
        public TaskFatalError(string what)
            : base(what)
        {
            Tidy = what;
        }
        public string Tidy { get; private set; }
    }

    public static class TaskManager
    {
        public static void AddCompleted(TaskFingerprint Fingerprint)
        {
            if (AllTasks.TryGetValue(Fingerprint, out var _))
                throw new ArgumentException($"Task with fingerprint {Fingerprint} already exists! Fingerprint should be unique to every task!");

            if (!AllTasks.TryAdd(Fingerprint, Task.FromResult(true)))
            {
                throw new ArgumentException($"Failed to add task with fingerprint {Fingerprint}! Are you adding same tasks in parallel?");
            }
        }

        public static Task<bool> Run(TaskFingerprint Fingerprint, Func<bool> Function)
        {
            if (AllTasks.TryGetValue(Fingerprint, out var _))
                throw new ArgumentException($"Task with fingerprint {Fingerprint} already exists! Fingerprint should be unique to every task!");

            var NewTask = Task.Run<bool>(
                () => {
                    if (StopAll)
                        return false;
                    return Function();
                })
                .ContinueWith(_ =>
                {
                    if (_.Exception?.InnerException is TaskFatalError fatal)
                    {
                        FatalErrors.Enqueue(fatal);
                        RootCTS.Cancel();
                        return false;
                    }
                    return _.Result;
                },
                TaskContinuationOptions.ExecuteSynchronously);

            if (!AllTasks.TryAdd(Fingerprint, NewTask))
            {
                throw new ArgumentException($"Failed to add task with fingerprint {Fingerprint}! Are you adding same tasks in parallel?");
            }

            return NewTask;
        }

        public static async Task<bool> AwaitFingerprint(TaskFingerprint Fingerprint)
        {
            Task<bool>? ToAwait = null;
            const int YieldThreshold = 1000;
            int YieldTimes = 0;
            while (!AllTasks.TryGetValue(Fingerprint, out ToAwait))
            {
                await Task.Yield();
                YieldTimes += 1;
                if (YieldTimes > YieldThreshold)
                    await Task.Delay(5);
            }
            return await ToAwait!;
        }

        public static void WaitAll(IEnumerable<Task> tasks)
        {
            Task.WaitAll(tasks);
        }

        public static void ForceQuit()
        {
            StopAll = true;
            WaitAll(AllTasks.Values);
        }

        internal static bool StopAll = false;
        internal static ConcurrentDictionary<TaskFingerprint, Task<bool>> AllTasks = new();
        internal static CancellationTokenSource RootCTS = new();
        internal static ConcurrentQueue<TaskFatalError> FatalErrors = new();
    }
}
