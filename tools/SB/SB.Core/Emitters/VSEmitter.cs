using SB.Core;
using System;
using System.Text;
using System.Xml.Linq;
using System.Collections.Concurrent;
using System.Linq;
using System.Collections.Generic;
using System.IO;

namespace SB
{
    using BS = BuildSystem;

    public class VSProjectInfo
    {
        public string ProjectPath { get; set; } = "";
        public string ProjectGuid { get; set; } = "";
        public HashSet<string> SourceFiles { get; } = new();
        public HashSet<string> HeaderFiles { get; } = new();
        public Dictionary<string, object?> CompileArguments { get; } = new();
        public List<string> IncludePaths { get; } = new();
        public List<string> Defines { get; } = new();
        public List<string> CompilerFlags { get; } = new();
    }

    public class VSEmitter : TaskEmitter
    {
        public VSEmitter(IToolchain Toolchain) => this.Toolchain = Toolchain;

        public static string OutputDirectory { get; set; } = ".sb/VisualStudio";
        public static string RootDirectory { get; set; } = "./";

        public override bool EnableEmitter(Target Target) => 
            Target.HasFilesOf<CppFileList>() || 
            Target.HasFilesOf<CFileList>() || 
            Target.HasFilesOf<ObjCppFileList>() || 
            Target.HasFilesOf<ObjCFileList>();

        public override bool EmitTargetTask(Target Target) => true;

        public override IArtifact? PerTargetTask(Target Target)
        {
            // Create output directory structure
            var relativeTargetPath = Path.GetRelativePath(RootDirectory, Target.Directory);
            var projectOutputDir = Path.Combine(OutputDirectory, relativeTargetPath);
            Directory.CreateDirectory(projectOutputDir);

            // Collect all source files and generate compile arguments
            var projectInfo = new VSProjectInfo
            {
                ProjectPath = Path.Combine(projectOutputDir, $"{Target.Name}.vcxproj"),
                ProjectGuid = GenerateGuid(Target.Name)
            };

            // Process all file lists
            ProcessFileList<CppFileList>(Target, projectInfo, CFamily.Cpp);
            ProcessFileList<CFileList>(Target, projectInfo, CFamily.C);
            ProcessFileList<ObjCppFileList>(Target, projectInfo, CFamily.ObjCpp);
            ProcessFileList<ObjCFileList>(Target, projectInfo, CFamily.ObjC);

            // Extract compile arguments using ArgumentDriver
            ExtractCompileArguments(Target, projectInfo);

            // Store project info for solution generation (must be done before generating project files)
            ProjectInfos.TryAdd(Target.Name, projectInfo);

            // Generate project files
            GenerateVcxproj(Target, projectInfo);
            GenerateVcxprojFilters(Target, projectInfo);

            return null;
        }

        private void ProcessFileList<T>(Target Target, VSProjectInfo projectInfo, CFamily language)
            where T : FileList, new()
        {
            var fileList = Target.FileList<T>();
            if (fileList != null)
            {
                foreach (var file in fileList.Files)
                {
                    var ext = Path.GetExtension(file).ToLower();
                    if (ext == ".cpp" || ext == ".cc" || ext == ".c" || ext == ".m" || ext == ".mm")
                    {
                        projectInfo.SourceFiles.Add(file);
                    }
                    else if (ext == ".h" || ext == ".hpp" || ext == ".hxx" || ext == ".inl")
                    {
                        projectInfo.HeaderFiles.Add(file);
                    }
                }
            }
        }

        private void ExtractCompileArguments(Target Target, VSProjectInfo projectInfo)
        {
            // Create a dummy ArgumentDriver to extract compile settings
            var driver = Toolchain.Compiler.CreateArgumentDriver(CFamily.Cpp, false)
                .AddArguments(Target.Arguments);

            // Store raw arguments for later use
            foreach (var kvp in Target.Arguments)
            {
                projectInfo.CompileArguments[kvp.Key] = kvp.Value;
            }

            // Extract common settings
            if (Target.Arguments.TryGetValue("IncludeDirs", out var includeDirs) && includeDirs is ArgumentList<string> includes)
            {
                projectInfo.IncludePaths.AddRange(includes);
            }

            if (Target.Arguments.TryGetValue("Defines", out var defines) && defines is ArgumentList<string> defs)
            {
                projectInfo.Defines.AddRange(defs);
            }

            if (Target.Arguments.TryGetValue("CppFlags", out var cppFlags) && cppFlags is ArgumentList<string> flags)
            {
                projectInfo.CompilerFlags.AddRange(flags);
            }
        }

        private void GenerateVcxproj(Target Target, VSProjectInfo projectInfo)
        {
            var ns = XNamespace.Get("http://schemas.microsoft.com/developer/msbuild/2003");
            
            var project = new XElement(ns + "Project",
                new XAttribute("DefaultTargets", "Build"),
                new XAttribute("ToolsVersion", "15.0"));

            // Project configurations
            var itemGroup = new XElement(ns + "ItemGroup",
                new XAttribute("Label", "ProjectConfigurations"));

            foreach (var config in new[] { "Debug", "Release" })
            {
                foreach (var platform in new[] { "x64", "Win32" })
                {
                    itemGroup.Add(new XElement(ns + "ProjectConfiguration",
                        new XAttribute("Include", $"{config}|{platform}"),
                        new XElement(ns + "Configuration", config),
                        new XElement(ns + "Platform", platform)));
                }
            }
            project.Add(itemGroup);

            // Globals
            project.Add(new XElement(ns + "PropertyGroup",
                new XAttribute("Label", "Globals"),
                new XElement(ns + "ProjectGuid", projectInfo.ProjectGuid),
                new XElement(ns + "RootNamespace", Target.Name),
                new XElement(ns + "WindowsTargetPlatformVersion", "10.0")));

            // Import default props
            project.Add(new XElement(ns + "Import",
                new XAttribute("Project", @"$(VCTargetsPath)\Microsoft.Cpp.Default.props")));

            // Configuration properties
            foreach (var config in new[] { "Debug", "Release" })
            {
                foreach (var platform in new[] { "x64", "Win32" })
                {
                    var condition = $"'$(Configuration)|$(Platform)'=='{config}|{platform}'";
                    project.Add(new XElement(ns + "PropertyGroup",
                        new XAttribute("Condition", condition),
                        new XAttribute("Label", "Configuration"),
                        new XElement(ns + "ConfigurationType", GetConfigurationType(Target)),
                        new XElement(ns + "UseDebugLibraries", config == "Debug" ? "true" : "false"),
                        new XElement(ns + "PlatformToolset", "v143"),
                        new XElement(ns + "CharacterSet", "Unicode")));
                }
            }

            // Import C++ props
            project.Add(new XElement(ns + "Import",
                new XAttribute("Project", @"$(VCTargetsPath)\Microsoft.Cpp.props")));

            // Build customizations
            AddBuildCustomizations(project, ns, Target);

            // Compiler settings
            AddCompilerSettings(project, ns, Target, projectInfo);

            // Source files - Use ClCompile for IntelliSense but with ExcludedFromBuild=true
            // This provides better IntelliSense while preventing actual compilation
            if (projectInfo.SourceFiles.Any())
            {
                var projectDir = Path.GetDirectoryName(projectInfo.ProjectPath) ?? "";
                project.Add(new XElement(ns + "ItemGroup",
                    projectInfo.SourceFiles.Select(file =>
                        new XElement(ns + "ClCompile",
                            new XAttribute("Include", GetRelativePath(projectDir, file)),
                            new XElement(ns + "ExcludedFromBuild", "true")))));
            }

            // Header files
            if (projectInfo.HeaderFiles.Any())
            {
                var projectDir = Path.GetDirectoryName(projectInfo.ProjectPath) ?? "";
                project.Add(new XElement(ns + "ItemGroup",
                    projectInfo.HeaderFiles.Select(file =>
                        new XElement(ns + "ClInclude",
                            new XAttribute("Include", GetRelativePath(projectDir, file))))));
            }

            // Import targets
            project.Add(new XElement(ns + "Import",
                new XAttribute("Project", @"$(VCTargetsPath)\Microsoft.Cpp.targets")));

            // Save project file
            var doc = new XDocument(
                new XDeclaration("1.0", "utf-8", null),
                project);

            doc.Save(projectInfo.ProjectPath);
        }

        private void GenerateVcxprojFilters(Target Target, VSProjectInfo projectInfo)
        {
            var ns = XNamespace.Get("http://schemas.microsoft.com/developer/msbuild/2003");
            var project = new XElement(ns + "Project",
                new XAttribute("ToolsVersion", "4.0"));

            // Create filter structure based on directory hierarchy
            var filters = new HashSet<string>();
            var sourceFilters = new Dictionary<string, string>();
            var headerFilters = new Dictionary<string, string>();

            var projectDir = Path.GetDirectoryName(projectInfo.ProjectPath) ?? "";

            // Process source files
            foreach (var file in projectInfo.SourceFiles)
            {
                var relativePath = GetRelativePath(projectDir, file);
                var dir = Path.GetDirectoryName(relativePath)?.Replace('\\', '/') ?? "";
                if (!string.IsNullOrEmpty(dir))
                {
                    filters.Add(dir);
                    sourceFilters[file] = dir;
                }
            }

            // Process header files
            foreach (var file in projectInfo.HeaderFiles)
            {
                var relativePath = GetRelativePath(projectDir, file);
                var dir = Path.GetDirectoryName(relativePath)?.Replace('\\', '/') ?? "";
                if (!string.IsNullOrEmpty(dir))
                {
                    filters.Add(dir);
                    headerFilters[file] = dir;
                }
            }

            // Add filters
            if (filters.Any())
            {
                project.Add(new XElement(ns + "ItemGroup",
                    filters.Select(filter =>
                        new XElement(ns + "Filter",
                            new XAttribute("Include", filter),
                            new XElement(ns + "UniqueIdentifier", $"{{{Guid.NewGuid()}}}")
                        ))));
            }

            // Add source files with filters - Use ClCompile for better IntelliSense
            if (sourceFilters.Any())
            {
                project.Add(new XElement(ns + "ItemGroup",
                    sourceFilters.Select(kvp =>
                        new XElement(ns + "ClCompile",
                            new XAttribute("Include", GetRelativePath(projectDir, kvp.Key)),
                            new XElement(ns + "Filter", kvp.Value)))));
            }

            // Add header files with filters
            if (headerFilters.Any())
            {
                project.Add(new XElement(ns + "ItemGroup",
                    headerFilters.Select(kvp =>
                        new XElement(ns + "ClInclude",
                            new XAttribute("Include", GetRelativePath(projectDir, kvp.Key)),
                            new XElement(ns + "Filter", kvp.Value)))));
            }

            // Save filters file
            var doc = new XDocument(project);
            doc.Save(projectInfo.ProjectPath + ".filters");
        }

        private void AddBuildCustomizations(XElement project, XNamespace ns, Target Target)
        {
            // Override MSBuild targets to use SB for actual building
            // Calculate relative path from project to engine root
            if (!ProjectInfos.TryGetValue(Target.Name, out var projectInfo))
            {
                throw new InvalidOperationException($"Project info for target '{Target.Name}' not found. Make sure to add project info before generating project files.");
            }
            
            // Add NMake settings for each configuration
            foreach (var config in new[] { "Debug", "Release" })
            {
                foreach (var platform in new[] { "x64", "Win32" })
                {
                    var condition = $"'$(Configuration)|$(Platform)'=='{config}|{platform}'";
                    var OutDir = Path.GetDirectoryName(GetTargetOutputPath(Target));
                    var customBuildGroup = new XElement(ns + "PropertyGroup",
                        new XAttribute("Condition", condition),
                        new XElement(ns + "NMakeBuildCommandLine", $"cd \"{RootDirectory}\" && dotnet run SB build --target={Target.Name} --mode={config.ToLower()}"),
                        new XElement(ns + "NMakeReBuildCommandLine", $"cd \"{RootDirectory}\" && dotnet run SB clean && dotnet run SB build --target={Target.Name} --mode={config.ToLower()}"),
                        new XElement(ns + "NMakeCleanCommandLine", $"cd \"{RootDirectory}\" && dotnet run SB clean"),
                        new XElement(ns + "NMakeOutput", GetTargetOutputPath(Target)),
                        new XElement(ns + "OutDir", OutDir + "\\"),
                        new XElement(ns + "IntDir", $"temp\\{Target.Name}\\{config}\\{platform}\\"),
                        new XElement(ns + "LocalDebuggerWorkingDirectory", OutDir + "\\"),
                        new XElement(ns + "DebuggerFlavor", "WindowsLocalDebugger"));

                    project.Add(customBuildGroup);
                }
            }
        }

        private void AddCompilerSettings(XElement project, XNamespace ns, Target Target, VSProjectInfo projectInfo)
        {
            foreach (var config in new[] { "Debug", "Release" })
            {
                foreach (var platform in new[] { "x64", "Win32" })
                {
                    var condition = $"'$(Configuration)|$(Platform)'=='{config}|{platform}'";
                    
                    // Add IntelliSense configuration for better IDE support
                    var intelliSenseGroup = new XElement(ns + "PropertyGroup",
                        new XAttribute("Condition", condition));

                    // Add include directories for IntelliSense
                    if (projectInfo.IncludePaths.Any())
                    {
                        var projectDir = Path.GetDirectoryName(projectInfo.ProjectPath) ?? "";
                        var relativeIncludes = projectInfo.IncludePaths.Select(inc => 
                            Path.IsPathFullyQualified(inc) ? GetRelativePath(projectDir, inc) : inc);
                        intelliSenseGroup.Add(new XElement(ns + "IncludePath",
                            string.Join(";", relativeIncludes) + ";$(IncludePath)"));
                    }

                    // Add preprocessor definitions for IntelliSense
                    if (projectInfo.Defines.Any())
                    {
                        intelliSenseGroup.Add(new XElement(ns + "NMakePreprocessorDefinitions",
                            string.Join(";", projectInfo.Defines) + ";$(NMakePreprocessorDefinitions)"));
                    }

                    // Add forced includes if any
                    if (projectInfo.CompilerFlags.Any(f => f.StartsWith("/FI") || f.StartsWith("-include")))
                    {
                        var forcedIncludes = projectInfo.CompilerFlags
                            .Where(f => f.StartsWith("/FI") || f.StartsWith("-include"))
                            .Select(f => f.StartsWith("/FI") ? f.Substring(3) : f.Substring(8))
                            .Where(f => !string.IsNullOrWhiteSpace(f));
                        
                        if (forcedIncludes.Any())
                        {
                            intelliSenseGroup.Add(new XElement(ns + "NMakeForcedIncludes",
                                string.Join(";", forcedIncludes)));
                        }
                    }

                    project.Add(intelliSenseGroup);
                }
            }
        }

        private string GetConfigurationType(Target Target)
        {
            // Always use Makefile to prevent VS from running native compilation
            // All actual compilation is handled by SB build system
            return "Makefile";
        }

        private string GetTargetOutputPath(Target Target)
        {
            var targetType = Target.GetTargetType();
            var extension = targetType switch
            {
                TargetType.Executable => ".exe",
                TargetType.Dynamic => ".dll",
                TargetType.Static => ".lib",
                _ => ""
            };
            return Path.Combine(Target.GetBinaryPath(), Target.Name + extension);
        }

        private string MapCppStandard(string version)
        {
            return version switch
            {
                "c++11" or "11" => "stdcpp11",
                "c++14" or "14" => "stdcpp14",
                "c++17" or "17" => "stdcpp17",
                "c++20" or "20" => "stdcpp20",
                "c++23" or "23" => "stdcpp23",
                "latest" => "stdcpplatest",
                _ => "stdcpp17" // Default
            };
        }

        private string GenerateGuid(string name)
        {
            // Generate deterministic GUID based on project name
            using var md5 = System.Security.Cryptography.MD5.Create();
            var hash = md5.ComputeHash(Encoding.UTF8.GetBytes(name));
            return "{" + new Guid(hash).ToString().ToUpper() + "}";
        }

        private string GetRelativePath(string basePath, string fullPath)
        {
            // Validate inputs
            if (string.IsNullOrEmpty(basePath) || string.IsNullOrEmpty(fullPath))
            {
                return fullPath ?? "";
            }

            // Normalize paths
            basePath = Path.GetFullPath(basePath);
            fullPath = Path.GetFullPath(fullPath);

            // Handle case where paths are on different drives (Windows)
            if (Path.GetPathRoot(basePath) != Path.GetPathRoot(fullPath))
            {
                return fullPath;
            }

            try
            {
                var baseUri = new Uri(basePath.EndsWith(Path.DirectorySeparatorChar.ToString()) 
                    ? basePath 
                    : basePath + Path.DirectorySeparatorChar);
                var fullUri = new Uri(fullPath);
                var relativeUri = baseUri.MakeRelativeUri(fullUri);
                return Uri.UnescapeDataString(relativeUri.ToString()).Replace('/', Path.DirectorySeparatorChar);
            }
            catch (Exception)
            {
                // If URI creation fails, fall back to returning the full path
                return fullPath;
            }
        }

        public static void GenerateSolution(string solutionPath, string solutionName)
        {
            // Ensure the solution directory exists
            var solutionDir = Path.GetDirectoryName(solutionPath);
            if (!string.IsNullOrEmpty(solutionDir))
            {
                Directory.CreateDirectory(solutionDir);
            }

            var sb = new StringBuilder();
            
            // Solution header
            sb.AppendLine("Microsoft Visual Studio Solution File, Format Version 12.00");
            sb.AppendLine("# Visual Studio Version 17");
            sb.AppendLine("VisualStudioVersion = 17.0.31903.59");
            sb.AppendLine("MinimumVisualStudioVersion = 10.0.40219.1");

            // Add projects
            foreach (var kvp in ProjectInfos)
            {
                var name = kvp.Key;
                var info = kvp.Value;
                
                // Calculate relative path safely
                string relativePath;
                try
                {
                    // Ensure we have valid paths
                    if (string.IsNullOrEmpty(solutionDir) || string.IsNullOrEmpty(info.ProjectPath))
                    {
                        relativePath = info.ProjectPath;
                    }
                    else
                    {
                        // Normalize paths
                        solutionDir = Path.GetFullPath(solutionDir);
                        var projectPath = Path.GetFullPath(info.ProjectPath);
                        
                        // Use Path.GetRelativePath which is safer than URI manipulation
                        relativePath = Path.GetRelativePath(solutionDir, projectPath);
                    }
                }
                catch (Exception)
                {
                    // Fall back to project path if relative path calculation fails
                    relativePath = info.ProjectPath;
                }
                
                sb.AppendLine($"Project(\"{{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}}\") = \"{name}\", \"{relativePath}\", \"{info.ProjectGuid}\"");
                sb.AppendLine("EndProject");
            }

            sb.AppendLine("Global");
            
            // Solution configurations
            sb.AppendLine("\tGlobalSection(SolutionConfigurationPlatforms) = preSolution");
            foreach (var config in new[] { "Debug", "Release" })
            {
                foreach (var platform in new[] { "x64", "Win32" })
                {
                    sb.AppendLine($"\t\t{config}|{platform} = {config}|{platform}");
                }
            }
            sb.AppendLine("\tEndGlobalSection");

            // Project configurations
            sb.AppendLine("\tGlobalSection(ProjectConfigurationPlatforms) = postSolution");
            foreach (var info in ProjectInfos.Values)
            {
                foreach (var config in new[] { "Debug", "Release" })
                {
                    foreach (var platform in new[] { "x64", "Win32" })
                    {
                        sb.AppendLine($"\t\t{info.ProjectGuid}.{config}|{platform}.ActiveCfg = {config}|{platform}");
                        sb.AppendLine($"\t\t{info.ProjectGuid}.{config}|{platform}.Build.0 = {config}|{platform}");
                    }
                }
            }
            sb.AppendLine("\tEndGlobalSection");

            sb.AppendLine("\tGlobalSection(SolutionProperties) = preSolution");
            sb.AppendLine("\t\tHideSolutionNode = FALSE");
            sb.AppendLine("\tEndGlobalSection");
            
            sb.AppendLine("EndGlobal");

            File.WriteAllText(solutionPath, sb.ToString());
        }


        private IToolchain Toolchain { get; }
        private static ConcurrentDictionary<string, VSProjectInfo> ProjectInfos = new();
    }
}