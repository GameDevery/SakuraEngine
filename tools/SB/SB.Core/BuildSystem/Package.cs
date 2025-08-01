﻿using System.Runtime.CompilerServices;

namespace SB.Core
{
    public record PackageConfig
    {
        public required Version Version { get; init; }
    }

    public class Package
    {
        public Package(string Name)
        {
            this.Name = Name;
        }

        public Package AvailableVersions(params Version[] Versions)
        {
            availableVersions.AddRange(Versions);
            return this;
        }

        public Package AddTarget(string TargetName, Action<Target, PackageConfig> Installer, [CallerFilePath] string? Loc = null, [CallerLineNumber] int LineNumber = 0)
        {
            if (Installers.TryGetValue(TargetName, out var _))
                throw new PackageInstallException(Name, TargetName, $"Package {Name}: Installer for target {TargetName} already exists!");

            Installers.Add(TargetName, new TargetInstaller { Action = Installer, Loc = Loc!, LineNumber = LineNumber });
            return this;
        }

        internal Target AcquireTarget(string TargetName, PackageConfig Config)
        {
            if (availableVersions.Count > 0 && !availableVersions.Contains(Config.Version))
                throw new PackageInstallException(Name, TargetName, $"Package {Name}: Version {Config.Version} not available!");

            Dictionary<PackageConfig, Target>? TargetPermutations;
            if (!AcquiredTargets.TryGetValue(TargetName, out TargetPermutations))
            {
                TargetPermutations = new();
                AcquiredTargets.Add(TargetName, TargetPermutations);
            }

            if (TargetPermutations.TryGetValue(Config, out var Permutation))
            {
                return Permutation;
            }
            else
            {
                TargetInstaller Installer;
                if (!Installers.TryGetValue(TargetName, out Installer))
                    throw new PackageInstallException(Name, TargetName, $"Package {Name}: Installer for target {TargetName} not found!");

                Target ToInstall = new Target($"{Name}@{TargetName}@{Config.Version}", true, Installer.Loc, Installer.LineNumber);
                Installer.Action(ToInstall, Config);
                TargetPermutations.Add(Config, ToInstall);
                return ToInstall;
            }
        }

        internal struct TargetInstaller
        {
            public required string Loc;
            public required int LineNumber;
            public Action<Target, PackageConfig> Action;
        }

        public string Name { get; private set; }
        internal HashSet<Version> availableVersions = new();
        internal Dictionary<string, TargetInstaller> Installers = new();
        internal Dictionary<string, Dictionary<PackageConfig, Target>> AcquiredTargets = new();
    }

    public class PackageInstallException : Exception
    {
        public PackageInstallException(string PackageName, string TargetName, string? message) 
            : base(message)
        {
        
        }
    }
}
