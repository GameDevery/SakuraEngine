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

        public static async Task Run(TaskFingerprint Fingerprint, Func<Task<bool>> Function, TaskScheduler? TS = null)
        {
            if (AllTasks.TryGetValue(Fingerprint, out var _))
                throw new ArgumentException($"Task with fingerprint {Fingerprint} already exists! Fingerprint should be unique to every task!");

            var NewTask = new Task<Task<bool>>(
                async () =>
                {
                    try
                    {
                        if (StopAll)
                            return await Task.FromResult(false);
                        return await Function();
                    }
                    catch (TaskFatalError fatal)
                    {
                        FatalErrors.Enqueue(fatal);
                        RootCTS.Cancel();
                        return await Task.FromResult(false);
                    }
                }, TaskCreationOptions.AttachedToParent | TaskCreationOptions.PreferFairness);
            NewTask.Start(TS is not null ? TS : TaskScheduler.Default);

            if (!AllTasks.TryAdd(Fingerprint, await NewTask))
            {
                throw new ArgumentException($"Failed to add task with fingerprint {Fingerprint}! Are you adding same tasks in parallel?");
            }
        }

        public static async Task<bool> AwaitFingerprint(TaskFingerprint Fingerprint)
        {
            Task<bool>? ToAwait = null;
            int YieldTimes = 0;
            while (!AllTasks.TryGetValue(Fingerprint, out ToAwait))
            {
                YieldTimes += 5;
                await Task.Delay(YieldTimes);
            }
            return await ToAwait!;
        }

        public static void WaitAll(IEnumerable<Task> tasks)
        {
            Task.WaitAll(tasks);
        }

        public static void WaitAll()
        {
            Task.WaitAll(AllTasks.Values);
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

    public class LimitedConcurrencyLevelTaskScheduler : TaskScheduler
    {
        // Indicates whether the current thread is processing work items.
        [ThreadStatic]
        private static bool _currentThreadIsProcessingItems;

        // The list of tasks to be executed
        private readonly LinkedList<Task> _tasks = new LinkedList<Task>(); // protected by lock(_tasks)

        // The maximum concurrency level allowed by this scheduler.
        private readonly int _maxDegreeOfParallelism;

        // Indicates whether the scheduler is currently processing work items.
        private int _delegatesQueuedOrRunning = 0;

        // Creates a new instance with the specified degree of parallelism.
        public LimitedConcurrencyLevelTaskScheduler(int maxDegreeOfParallelism)
        {
            if (maxDegreeOfParallelism < 1) throw new ArgumentOutOfRangeException("maxDegreeOfParallelism");
            _maxDegreeOfParallelism = maxDegreeOfParallelism;
        }

        // Queues a task to the scheduler.
        protected sealed override void QueueTask(Task task)
        {
            // Add the task to the list of tasks to be processed.  If there aren't enough
            // delegates currently queued or running to process tasks, schedule another.
            lock (_tasks)
            {
                _tasks.AddLast(task);
                if (_delegatesQueuedOrRunning < _maxDegreeOfParallelism)
                {
                    ++_delegatesQueuedOrRunning;
                    NotifyThreadPoolOfPendingWork();
                }
            }
        }

        // Inform the ThreadPool that there's work to be executed for this scheduler.
        private void NotifyThreadPoolOfPendingWork()
        {
            ThreadPool.UnsafeQueueUserWorkItem(_ =>
            {
                // Note that the current thread is now processing work items.
                // This is necessary to enable inlining of tasks into this thread.
                _currentThreadIsProcessingItems = true;
                try
                {
                    // Process all available items in the queue.
                    while (true)
                    {
                        Task item;
                        lock (_tasks)
                        {
                            // When there are no more items to be processed,
                            // note that we're done processing, and get out.
                            if (_tasks.Count == 0)
                            {
                                --_delegatesQueuedOrRunning;
                                break;
                            }

                            // Get the next item from the queue
                            item = _tasks.First.Value;
                            _tasks.RemoveFirst();
                        }

                        // Execute the task we pulled out of the queue
                        base.TryExecuteTask(item);
                    }
                }
                // We're done processing items on the current thread
                finally { _currentThreadIsProcessingItems = false; }
            }, null);
        }

        // Attempts to execute the specified task on the current thread.
        protected sealed override bool TryExecuteTaskInline(Task task, bool taskWasPreviouslyQueued)
        {
            // If this thread isn't already processing a task, we don't support inlining
            if (!_currentThreadIsProcessingItems) return false;

            // If the task was previously queued, remove it from the queue
            if (taskWasPreviouslyQueued)
                // Try to run the task.
                if (TryDequeue(task))
                    return base.TryExecuteTask(task);
                else
                    return false;
            else
                return base.TryExecuteTask(task);
        }

        // Attempt to remove a previously scheduled task from the scheduler.
        protected sealed override bool TryDequeue(Task task)
        {
            lock (_tasks) return _tasks.Remove(task);
        }

        // Gets the maximum concurrency level supported by this scheduler.
        public sealed override int MaximumConcurrencyLevel { get { return _maxDegreeOfParallelism; } }

        // Gets an enumerable of the tasks currently scheduled on this scheduler.
        protected sealed override IEnumerable<Task> GetScheduledTasks()
        {
            bool lockTaken = false;
            try
            {
                Monitor.TryEnter(_tasks, ref lockTaken);
                if (lockTaken) return _tasks;
                else throw new NotSupportedException();
            }
            finally
            {
                if (lockTaken) Monitor.Exit(_tasks);
            }
        }
    }
}
