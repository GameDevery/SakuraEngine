using SB.Core;
using Serilog;
using System.IO;
using System.Linq;

namespace SB
{
    using BS = BuildSystem;

    /// <summary>
    /// Attribute to mark targets for artifact installation
    /// </summary>
    public class InstallArtifactAttribute
    {
        /// <summary>
        /// Destination directory for installation (absolute path or relative to project root)
        /// </summary>
        public string? InstallDirectory { get; set; }
        
        /// <summary>
        /// Whether to install PDB files alongside executables/DLLs
        /// </summary>
        public bool InstallPDB { get; set; } = true;
        
        /// <summary>
        /// Whether to install the artifact
        /// </summary>
        public bool Enable { get; set; } = true;
    }

    /// <summary>
    /// TaskEmitter for installing build artifacts (EXE, DLL, PDB) to specified directories
    /// </summary>
    public class InstallArtifactEmitter : TaskEmitter
    {
        public override bool EnableEmitter(Target Target) 
        {
            var attr = Target.GetAttribute<InstallArtifactAttribute>();
            return attr != null && attr.Enable;
        }
        
        public override bool EmitTargetTask(Target Target) => true;
        
        public override IArtifact? PerTargetTask(Target Target)
        {
            var attr = Target.GetAttribute<InstallArtifactAttribute>();
            if (attr == null || !attr.Enable)
                return null;
            
            // Wait for link result
            var linkResults = BS.Artifacts.OfType<LinkResult>()
                .Where(a => a.Target == Target)
                .ToList();
            
            if (!linkResults.Any())
            {
                Log.Verbose("No link result found for target {TargetName}, skipping installation", Target.Name);
                return null;
            }
            
            var linkResult = linkResults.First();
            
            // Don't install if the artifact was restored from cache
            if (linkResult.IsRestored)
            {
                Log.Verbose("Target {TargetName} was restored from cache, skipping installation", Target.Name);
                return null;
            }
            
            // Determine installation directory
            string installDir;
            if (!string.IsNullOrEmpty(attr.InstallDirectory))
            {
                installDir = Path.IsPathFullyQualified(attr.InstallDirectory)
                    ? attr.InstallDirectory
                    : Path.Combine(BS.TempPath, attr.InstallDirectory);
            }
            else
            {
                // Default installation directory based on target category
                if (Target.IsCategory(TargetCategory.Tool))
                {
                    installDir = Path.Combine(BS.TempPath, "tools");
                }
                else if (Target.IsCategory(TargetCategory.Runtime))
                {
                    installDir = Path.Combine(BS.BuildPath, "bin");
                }
                else
                {
                    // Skip installation for other categories unless explicitly configured
                    Log.Verbose("No installation directory specified for target {TargetName}, skipping", Target.Name);
                    return null;
                }
            }
            
            // Ensure installation directory exists
            Directory.CreateDirectory(installDir);
            
            // Install main artifact (EXE or DLL)
            if (File.Exists(linkResult.TargetFile))
            {
                var targetFileName = Path.GetFileName(linkResult.TargetFile);
                var destinationFile = Path.Combine(installDir, targetFileName);
                
                Log.Information("Installing {TargetFile} to {InstallDir}", targetFileName, installDir);
                File.Copy(linkResult.TargetFile, destinationFile, overwrite: true);
            }
            
            // Install PDB file if requested and available
            if (attr.InstallPDB && !string.IsNullOrEmpty(linkResult.PDBFile) && File.Exists(linkResult.PDBFile))
            {
                var pdbFileName = Path.GetFileName(linkResult.PDBFile);
                var destinationPDB = Path.Combine(installDir, pdbFileName);
                
                Log.Verbose("Installing PDB {PDBFile} to {InstallDir}", pdbFileName, installDir);
                File.Copy(linkResult.PDBFile, destinationPDB, overwrite: true);
            }
            
            return new InstallResult
            {
                Target = Target,
                InstallDirectory = installDir,
                IsRestored = false
            };
        }
    }
    
    /// <summary>
    /// Result of artifact installation
    /// </summary>
    public struct InstallResult : IArtifact
    {
        public required Target Target { get; init; }
        public required string InstallDirectory { get; init; }
        public bool IsRestored { get; init; }
    }
    
    /// <summary>
    /// Extension methods for configuring artifact installation
    /// </summary>
    public static partial class TargetExtensions
    {
        /// <summary>
        /// Configure the target to install its artifacts to a specific directory
        /// </summary>
        public static Target InstallArtifactTo(this Target @this, string installDirectory, bool installPDB = true)
        {
            @this.SetAttribute(new InstallArtifactAttribute
            {
                InstallDirectory = installDirectory,
                InstallPDB = installPDB,
                Enable = true
            });
            return @this;
        }
        
        /// <summary>
        /// Configure the target to install its artifacts using default directory based on category
        /// </summary>
        public static Target InstallArtifact(this Target @this, bool installPDB = true)
        {
            @this.SetAttribute(new InstallArtifactAttribute
            {
                InstallPDB = installPDB,
                Enable = true
            });
            return @this;
        }
        
        /// <summary>
        /// Disable artifact installation for this target
        /// </summary>
        public static Target NoInstallArtifact(this Target @this)
        {
            @this.SetAttribute(new InstallArtifactAttribute
            {
                Enable = false
            });
            return @this;
        }
    }
}