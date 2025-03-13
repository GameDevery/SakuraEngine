using Microsoft.Extensions.FileSystemGlobbing;
using SB.Core;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;

namespace SB
{
    public class TargetDependArgumentDriver : IArgumentDriver
    {
        [TargetProperty(TargetProperty.InheritBehavior)]
        public ArgumentList<string> Depend(ArgumentList<string> Deps) => Deps;

        public Dictionary<string, object?> Arguments { get; }
        public HashSet<string> RawArguments { get; }
    }

    public class Target : TargetSetters
    {
        internal Target(string Name, [CallerFilePath] string Location = null)
        {
            this.Name = Name;
            this.Location = Location;
            this.Directory = Path.GetDirectoryName(Location);
        }

        public Target AddFiles(params string[] files)
        {
            foreach (var file in files)
            {
                if (file.Contains("*"))
                    Globs.Add(file);
                else if (Path.IsPathFullyQualified(file))
                    Absolutes.Add(file);
                else
                    Absolutes.Add(Path.Combine(Directory, file));
            }
            return this;
        }

        public Target Depend(string DependName)
        {
            if (DependName.Contains("@"))
                PackageTargetDependencies.Add(DependName);
            else
                TargetDependencies.Add(DependName);
            return this;
        }

        public Target Require(string Package, PackageConfig Config)
        {
            if (PackageDependencies.TryGetValue(Package, out var _))
                throw new ArgumentException($"Target {Name}: Required package {Package} is already required!");
            PackageDependencies.Add(Package, Config);
            return this;
        }

        public Target BeforeBuild(Action<Target> Action)
        {
            BeforeBuildActions.Add(Action);
            return this;
        }

        private bool PackagesResolved = false;
        internal void ResolvePackages(ref Dictionary<string, Target> OutPackageTargets)
        {
            if (!PackagesResolved)
            {
                foreach (var KVP in PackageDependencies)
                {
                    var PackageName = KVP.Key;
                    var PackageConfig = KVP.Value;
                    var Package = BuildSystem.GetPackage(PackageName);
                    if (Package == null)
                        throw new ArgumentException($"Target {Name}: Required package {PackageName} does not exist!");
                
                    foreach (var PackageTargetDependency in PackageTargetDependencies)
                    {
                        var Splitted = PackageTargetDependency.Split("@");
                        if (Splitted[0] == PackageName)
                        {
                            var PackageTarget = Package.AcquireTarget(Splitted[1], PackageConfig);
                            {
                                PackageTarget.ResolvePackages(ref OutPackageTargets);
                                OutPackageTargets.TryAdd(PackageTargetDependency, PackageTarget);
                            }
                            TargetDependencies.Add(PackageTargetDependency);
                        }
                    }
                }
                PackagesResolved = true;
            }
        }

        internal void ResolveDependencies() => RecursiveMergeDependencies(TargetDependencies, TargetDependencies.ToHashSet());
        internal void RecursiveMergeDependencies(ISet<string> To, IReadOnlySet<string> DepNames)
        {
            foreach (var DepName in DepNames)
            {
                Target DepTarget = BuildSystem.GetTarget(DepName);
                To.AddRange(DepTarget.Dependencies);
                RecursiveMergeDependencies(To, DepTarget.Dependencies);
            }
        }

        internal void ResolveArguments()
        {
            // Files
            if (Globs.Count != 0)
            {
                var GlobMatcher = new Matcher();
                GlobMatcher.AddIncludePatterns(Globs);
                Absolutes.AddRange(GlobMatcher.GetResultsInFullPath(Directory));
            }
            // Arguments
            MergeArguments(FinalArguments, PublicArguments);
            MergeArguments(FinalArguments, PrivateArguments);
            foreach (var DepName in Dependencies)
            {
                Target DepTarget = BuildSystem.GetTarget(DepName);
                MergeArguments(FinalArguments, DepTarget.PublicArguments);
                MergeArguments(FinalArguments, DepTarget.InterfaceArguments);
            }
        }

        internal void MergeArguments(Dictionary<string, object?> To, Dictionary<string, object?> From)
        {
            foreach (var KVP in From)
            {
                var K = KVP.Key;
                var V = KVP.Value;
                if (V is IArgumentList)
                {
                    var ArgList = V as IArgumentList;
                    if (To.TryGetValue(K, out var Existed))
                        (Existed as IArgumentList).Merge(ArgList);
                    else
                        To.Add(K, ArgList.Copy());
                }
                else
                {
                    if (To.TryGetValue(K, out var Existed))
                    {
                        To.Add(K, V);
                    }
                }
            }
        }

        public IReadOnlySet<string> Dependencies => TargetDependencies;
        public IReadOnlySet<string> AllFiles => Absolutes;

        public string Name { get; }
        public string Location { get; }
        public string Directory { get; }
        #region Files
        private SortedSet<string> Globs = new();
        private SortedSet<string> Absolutes = new();
        #endregion
        #region Dependencies
        private SortedDictionary<string, PackageConfig> PackageDependencies = new();
        private SortedSet<string> TargetDependencies = new();
        private SortedSet<string> PackageTargetDependencies = new();
        #endregion
        #region Package
        public bool IsFromPackage { get; internal set; } = false;
        #endregion
        internal List<Action<Target>> BeforeBuildActions = new();
    }
}
