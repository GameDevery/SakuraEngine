using Microsoft.Extensions.FileSystemGlobbing;
using SB.Core;
using System.Runtime.CompilerServices;

namespace SB
{
    public partial class Target : TargetSetters
    {
        internal Target(string Name, [CallerFilePath] string? Location = null)
        {
            this.Name = Name;
            this.Location = Location!;
            this.Directory = Path.GetDirectoryName(Location)!;
            BuildSystem.TargetDefaultSettings(this);
        }

        private System.Threading.Lock FileOperationsLock = new();
        public Target AddFiles(params string[] files)
        {
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

        public Target RemoveFiles(params string[] files)
        {
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

        public Target Depend(Visibility Visibility, params string[] DependNames)
        {
            foreach (var DependName in DependNames)
            {
                switch (Visibility)
                {
                    case Visibility.Public:
                        {
                            if (DependName.Contains("@"))
                                PublicPackageTargetDependencies.Add(DependName);
                            else
                                PublicTargetDependencies.Add(DependName);
                        }
                        break;
                    case Visibility.Private:
                        {
                            if (DependName.Contains("@"))
                                PrivatePackageTargetDependencies.Add(DependName);
                            else
                                PrivateTargetDependencies.Add(DependName);
                        }
                        break;
                    case Visibility.Interface:
                        {
                            if (DependName.Contains("@"))
                                InterfacePackageTargetDependencies.Add(DependName);
                            else
                                InterfaceTargetDependencies.Add(DependName);
                        }
                        break;
                }
            }
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

        public Target AfterLoad(Action<Target> Action)
        {
            AfterLoadActions.Add(Action);
            return this;
        }

        public Target SetAttribute<T>(T Attribute)
        {
            Attributes.Add(typeof(T), Attribute);
            return this;
        }

        public T? GetAttribute<T>()
        {
            if (Attributes.TryGetValue(typeof(T), out var Attribute))
                return (T?)Attribute;
            return default;
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

                    var ResolvePackageTargetDependencies =
                        (SortedSet<string> PackageTargetDependencies, ref SortedSet<string> ResolvedTargetDependencies, ref Dictionary<string, Target> OutPackageTargets) =>
                        {
                            foreach (var NickName in PackageTargetDependencies)
                            {
                                var Splitted = NickName.Split("@");
                                if (Splitted[0] == PackageName)
                                {
                                    var PackageTarget = Package.AcquireTarget(Splitted[1], PackageConfig);
                                    {
                                        PackageTarget.ResolvePackages(ref OutPackageTargets);
                                        OutPackageTargets.TryAdd(PackageTarget.Name, PackageTarget);
                                    }
                                    ResolvedTargetDependencies.Add(PackageTarget.Name);
                                }
                            }
                        };

                    ResolvePackageTargetDependencies(PrivatePackageTargetDependencies, ref PrivateTargetDependencies, ref OutPackageTargets);
                    ResolvePackageTargetDependencies(PublicPackageTargetDependencies, ref PublicTargetDependencies, ref OutPackageTargets);
                    ResolvePackageTargetDependencies(InterfacePackageTargetDependencies, ref InterfaceTargetDependencies, ref OutPackageTargets);
                }
                PackagesResolved = true;
            }
        }

        internal void ResolveDependencies()
        {
            RecursiveMergeDependencies(FinalTargetDependencies, PublicTargetDependencies);
            RecursiveMergeDependencies(FinalTargetDependencies, PrivateTargetDependencies);
        }

        internal void RecursiveMergeDependencies(ISet<string> To, IReadOnlySet<string> DepNames)
        {
            To.AddRange(DepNames);
            foreach (var DepName in DepNames)
            {
                Target DepTarget = BuildSystem.GetTarget(DepName);
                RecursiveMergeDependencies(To, DepTarget.PublicTargetDependencies);
                RecursiveMergeDependencies(To, DepTarget.InterfaceTargetDependencies);
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
                        (Existed as IArgumentList)!.Merge(ArgList!);
                    else
                        To.Add(K, ArgList!.Copy());
                }
                else
                {
                    if (!To.TryGetValue(K, out var Existed))
                    {
                        To.Add(K, V);
                    }
                    else
                        throw new TaskFatalError("Argument Confict!");
                }
            }
        }

        private Dictionary<Type, object?> Attributes = new();
        public IReadOnlySet<string> Dependencies => FinalTargetDependencies;
        public IReadOnlySet<string> PublicDependencies => PublicTargetDependencies;
        public IReadOnlySet<string> PrivateDependencies => PrivateTargetDependencies;
        public IReadOnlySet<string> InterfaceDependencies => InterfaceTargetDependencies;
        public IReadOnlySet<string> AllFiles => Absolutes;

        public string Name { get; }
        public string Location { get; }
        public string Directory { get; }

        #region Files
        private SortedSet<string> Globs = new();
        private SortedSet<string> Absolutes = new();
        private System.Threading.Lock FilesOperationLock = new();
        #endregion
        
        #region Dependencies
        private SortedDictionary<string, PackageConfig> PackageDependencies = new();

        private SortedSet<string> FinalTargetDependencies = new();
        private SortedSet<string> PublicTargetDependencies = new();
        private SortedSet<string> PrivateTargetDependencies = new();
        private SortedSet<string> InterfaceTargetDependencies = new();

        private SortedSet<string> PublicPackageTargetDependencies = new();
        private SortedSet<string> PrivatePackageTargetDependencies = new();
        private SortedSet<string> InterfacePackageTargetDependencies = new();
        #endregion

        #region Package
        public bool IsFromPackage { get; internal set; } = false;
        #endregion

        internal List<Action<Target>> BeforeBuildActions = new();
        internal List<Action<Target>> AfterLoadActions = new();
    }

    public static partial class TargetExtensions
    {
        public static TargetType? GetTargetType(this Target Target)
        {
            if (Target.Arguments.TryGetValue("TargetType", out var V))
                return (TargetType?)V;
            return null;
        }
    }
}
