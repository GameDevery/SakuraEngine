using System.Collections.Concurrent;
using System.Reflection;
using System.Diagnostics;
using Serilog;

namespace SB
{
    public interface ISetup
    {
        public abstract void Setup();
    }

    public abstract class SetupAttribute : Attribute {}

    [AttributeUsage(AttributeTargets.Class, Inherited = true, AllowMultiple = true)]
    public class SetupAttribute<T> : SetupAttribute
        where T : ISetup, new()
    {
        public SetupAttribute()
        {
            BuildSystem.AddSetup<T>();
        }
    }

    public partial class BuildSystem
    {
        public static void RunSetups(bool IgnoreSetupAttributes = false)
        {
            using (Profiler.BeginZone("RunSetups", color: (uint)Profiler.ColorType.WebMaroon))
            {
                if (!IgnoreSetupAttributes)
                {
                    var SetupAttributes = new ConcurrentBag<SetupAttribute>();
                    foreach (var Assembly in AppDomain.CurrentDomain.GetAssemblies())
                    {
                        if (!Assembly.GetReferencedAssemblies().Any(A => A.Name == "SB.Core") && Assembly.GetName().Name != "SB.Core")
                            continue;

                        foreach (var Type in Assembly.GetTypes())
                        {
                            var SetupAttrs = Type.GetCustomAttributes<SetupAttribute>();
                            foreach (var SetupAttr in SetupAttrs)
                            {
                                SetupAttributes.Add(SetupAttr);
                            }
                        }
                    }
                }

                Log.Verbose("Starting setups with {ProcessorCount} threads ...", Environment.ProcessorCount);
                Parallel.ForEach(_AllSetups,
                new ParallelOptions { MaxDegreeOfParallelism = Environment.ProcessorCount },
                (setup) =>
                {
                    using (Profiler.BeginZone($"{setup.GetType().Name}", color: (uint)Profiler.ColorType.WebMaroon))
                    {
                        Stopwatch sw = new();
                        sw.Start();
                        Log.Verbose("Setup {Name} starts ...", setup.GetType().Name);
                        try
                        {
                            setup.Setup();
                        }
                        catch (Exception ex)
                        {
                            Log.Fatal("Setup {Name} failed: {Message}", setup.GetType().Name, ex.Message);
                            throw;
                        }
                        sw.Stop();
                        float Seconds = sw.ElapsedMilliseconds / 1000.0f;
                        Log.Verbose("Setup {Name} finished... cost {Seconds}s", setup.GetType().Name, Seconds);
                    }
                });
                _AllSetups.Clear();
            }
        }

        public static void AddSetup<T>()
            where T : ISetup, new()
        {
            var setup = new T();
            _AllSetups.Add(setup);
        }

        private static ConcurrentBag<ISetup> _AllSetups = new ConcurrentBag<ISetup>();
    }
}