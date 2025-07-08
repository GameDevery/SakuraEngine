using SB.Core;
using System;
using System.Text;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Serilog;

namespace SB
{
    /// <summary>
    /// Generates dependency graph visualization for the entire project
    /// Outputs DOT format files that can be rendered with Graphviz
    /// </summary>
    public class DependencyGraphEmitter : TaskEmitter
    {
        public enum GraphFormat { DOT, Mermaid, PlantUML }
        public enum NodeColorScheme { ByType, ByModule, BySize, ByDependencyCount }
        
        // Configuration
        public static string OutputDirectory { get; set; } = ".sb/graphs";
        public static GraphFormat Format { get; set; } = GraphFormat.DOT;
        public static NodeColorScheme ColorScheme { get; set; } = NodeColorScheme.ByType;
        public static bool IncludeExternalDeps { get; set; } = false;
        public static bool ShowTransitiveDeps { get; set; } = false;
        public static int MaxDepth { get; set; } = -1; // -1 means unlimited
        
        private static readonly Dictionary<string, TargetInfo> targetInfos = new();
        private static readonly HashSet<(string from, string to)> edges = new();
        private static readonly object syncLock = new object();
        
        private class TargetInfo
        {
            public Target Target { get; set; } = null!;
            public long CodeSize { get; set; }
            public int SourceFileCount { get; set; }
            public int DirectDependencyCount { get; set; }
            public int TransitiveDependencyCount { get; set; }
            public string ModulePath { get; set; } = "";
            public List<string> ExternalDependencies { get; set; } = new();
        }
        
        public override bool EnableEmitter(Target Target) => true;
        public override bool EmitTargetTask(Target Target) => true;
        
        public static void Clear()
        {
            lock (syncLock)
            {
                targetInfos.Clear();
                edges.Clear();
            }
        }
        
        public override IArtifact? PerTargetTask(Target Target)
        {
            var info = CollectTargetInfo(Target);
            var deps = CollectDependencies(Target);
            
            lock (syncLock)
            {
                targetInfos[Target.Name] = info;
                foreach (var depName in deps)
                {
                    edges.Add((Target.Name, depName));
                }
            }
            
            return null;
        }
        
        private TargetInfo CollectTargetInfo(Target target)
        {
            var info = new TargetInfo
            {
                Target = target,
                ModulePath = GetModulePath(target),
                SourceFileCount = CountSourceFiles(target),
                CodeSize = EstimateCodeSize(target),
                ExternalDependencies = GetExternalDependencies(target)
            };
            
            // Always count only direct dependencies for the label
            info.DirectDependencyCount = target.PublicDependencies.Count + 
                                       target.PrivateDependencies.Count + 
                                       target.InterfaceDependencies.Count;
            
            return info;
        }
        
        private HashSet<string> CollectDependencies(Target target)
        {
            var deps = new HashSet<string>();
            
            if (ShowTransitiveDeps)
            {
                // Use all dependencies including transitive ones
                foreach (var depName in target.Dependencies)
                    deps.Add(depName);
            }
            else
            {
                // Only use direct dependencies
                foreach (var depName in target.PublicDependencies)
                    deps.Add(depName);
                foreach (var depName in target.PrivateDependencies)
                    deps.Add(depName);
                foreach (var depName in target.InterfaceDependencies)
                    deps.Add(depName);
            }
            
            return deps;
        }
        
        public static void GenerateDependencyGraph()
        {
            // Ensure we have data to process
            if (targetInfos.Count == 0)
            {
                Log.Warning("No targets collected. Make sure to run build before generating graph.");
                return;
            }
            
            // Calculate transitive dependencies
            CalculateTransitiveDependencies();
            
            // Generate graph files
            Directory.CreateDirectory(OutputDirectory);
            
            var timestamp = DateTime.Now.ToString("yyyyMMdd_HHmmss");
            var filename = $"dependency_graph_{timestamp}";
            
            // Create thread-safe copies for rendering
            Dictionary<string, TargetInfo> targetInfosCopy;
            HashSet<(string from, string to)> edgesCopy;
            lock (syncLock)
            {
                targetInfosCopy = new Dictionary<string, TargetInfo>(targetInfos);
                edgesCopy = new HashSet<(string from, string to)>(edges);
            }
            
            // Generate graph based on format
            var outputPath = Path.Combine(OutputDirectory, filename);
            switch (Format)
            {
                case GraphFormat.DOT:
                    new DotRenderer().Render(targetInfosCopy, edgesCopy, $"{outputPath}.dot");
                    break;
                case GraphFormat.Mermaid:
                    new MermaidRenderer().Render(targetInfosCopy, edgesCopy, $"{outputPath}.mmd");
                    break;
                case GraphFormat.PlantUML:
                    new PlantUMLRenderer().Render(targetInfosCopy, edgesCopy, $"{outputPath}.puml");
                    break;
            }
            
            // Generate statistics report
            GenerateStatisticsReport(targetInfosCopy, edgesCopy, $"{outputPath}_stats.txt");
            
            Log.Information($"Generated dependency graph with {targetInfosCopy.Count} targets and {edgesCopy.Count} edges");
        }
        
        #region Graph Renderers
        
        private abstract class GraphRenderer
        {
            public abstract void Render(Dictionary<string, TargetInfo> targets, HashSet<(string from, string to)> edges, string path);
            
            protected string GenerateNodeLabel(string name, TargetInfo info)
            {
                var lines = new List<string> { name };
                
                lines.Add($"Type: {info.Target.GetTargetType()}");
                lines.Add($"Files: {info.SourceFileCount}");
                
                if (info.CodeSize > 0)
                    lines.Add($"Size: {FormatSize(info.CodeSize)}");
                    
                lines.Add($"Deps: {info.DirectDependencyCount}");
                
                return string.Join("\\n", lines);
            }
            
            protected void WriteToFile(string path, string content)
            {
                File.WriteAllText(path, content);
                Log.Information($"Generated {GetType().Name.Replace("Renderer", "")} file: {path}");
            }
        }
        
        private class DotRenderer : GraphRenderer
        {
            public override void Render(Dictionary<string, TargetInfo> targets, HashSet<(string from, string to)> edges, string path)
            {
                var sb = new StringBuilder();
                sb.AppendLine("digraph DependencyGraph {");
                sb.AppendLine("    rankdir=LR;");
                sb.AppendLine("    node [shape=box, style=\"rounded,filled\", fontname=\"Arial\"];");
                sb.AppendLine("    edge [fontname=\"Arial\", fontsize=10];");
                sb.AppendLine();
                
                // Generate nodes
                foreach (var (name, info) in targets)
                {
                    var color = GetNodeColor(info);
                    var label = GenerateNodeLabel(name, info);
                    var shape = GetNodeShape(info);
                    
                    sb.AppendLine($"    \"{name}\" [label=\"{label}\", fillcolor=\"{color}\", shape={shape}];");
                }
                
                sb.AppendLine();
                
                // Generate edges
                foreach (var (from, to) in edges)
                {
                    sb.AppendLine($"    \"{from}\" -> \"{to}\" [color=\"darkgray\"];");
                }
                
                // Add legend
                AddDotLegend(sb);
                
                sb.AppendLine("}");
                
                WriteToFile(path, sb.ToString());
            }
            
            private void AddDotLegend(StringBuilder sb)
            {
                sb.AppendLine();
                sb.AppendLine("    // Legend");
                sb.AppendLine("    subgraph cluster_legend {");
                sb.AppendLine("        label=\"Legend\";");
                sb.AppendLine("        style=dashed;");
                
                switch (ColorScheme)
                {
                    case NodeColorScheme.ByType:
                        sb.AppendLine("        \"Static Library\" [fillcolor=\"lightblue\"];");
                        sb.AppendLine("        \"Dynamic Library\" [fillcolor=\"lightgreen\"];");
                        sb.AppendLine("        \"Executable\" [fillcolor=\"lightyellow\"];");
                        break;
                    case NodeColorScheme.ByModule:
                        sb.AppendLine("        \"Core Module\" [fillcolor=\"lightcoral\"];");
                        sb.AppendLine("        \"Engine Module\" [fillcolor=\"lightblue\"];");
                        sb.AppendLine("        \"Game Module\" [fillcolor=\"lightgreen\"];");
                        break;
                }
                
                sb.AppendLine("    }");
            }
        }
        
        private class MermaidRenderer : GraphRenderer
        {
            public override void Render(Dictionary<string, TargetInfo> targets, HashSet<(string from, string to)> edges, string path)
            {
                var sb = new StringBuilder();
                sb.AppendLine("graph LR");
                sb.AppendLine();
                
                // Generate nodes with subgraphs for modules
                var moduleGroups = targets.GroupBy(t => t.Value.ModulePath);
                foreach (var module in moduleGroups)
                {
                    if (!string.IsNullOrEmpty(module.Key))
                    {
                        sb.AppendLine($"    subgraph {SanitizeMermaidId(module.Key)}[\"{module.Key}\"]");
                    }
                    
                    foreach (var (name, info) in module)
                    {
                        var label = GenerateNodeLabel(name, info);
                        var style = GetMermaidNodeStyle(info);
                        sb.AppendLine($"        {SanitizeMermaidId(name)}[\"{label}\"]:::{style}");
                    }
                    
                    if (!string.IsNullOrEmpty(module.Key))
                    {
                        sb.AppendLine("    end");
                    }
                }
                
                sb.AppendLine();
                
                // Generate edges
                foreach (var (from, to) in edges)
                {
                    sb.AppendLine($"    {SanitizeMermaidId(from)} --> {SanitizeMermaidId(to)}");
                }
                
                // Add styles
                sb.AppendLine();
                sb.AppendLine("    classDef executable fill:#ffffcc,stroke:#333,stroke-width:2px;");
                sb.AppendLine("    classDef static fill:#ccccff,stroke:#333,stroke-width:2px;");
                sb.AppendLine("    classDef dynamic fill:#ccffcc,stroke:#333,stroke-width:2px;");
                
                WriteToFile(path, sb.ToString());
            }
            
            private string SanitizeMermaidId(string id)
            {
                return id.Replace('-', '_').Replace('/', '_');
            }
            
            private string GetMermaidNodeStyle(TargetInfo info)
            {
                return info.Target.GetTargetType() switch
                {
                    TargetType.Executable => "executable",
                    TargetType.Dynamic => "dynamic",
                    _ => "static"
                };
            }
        }
        
        private class PlantUMLRenderer : GraphRenderer
        {
            public override void Render(Dictionary<string, TargetInfo> targets, HashSet<(string from, string to)> edges, string path)
            {
                var sb = new StringBuilder();
                sb.AppendLine("@startuml");
                sb.AppendLine("!define RECTANGLE class");
                sb.AppendLine("skinparam componentStyle rectangle");
                sb.AppendLine();
                
                // Generate components
                foreach (var (name, info) in targets)
                {
                    var stereotype = GetPlantUMLStereotype(info);
                    var label = GenerateNodeLabel(name, info);
                    sb.AppendLine($"component \"{label}\" as {SanitizePlantUMLId(name)} {stereotype}");
                }
                
                sb.AppendLine();
                
                // Generate relationships
                foreach (var (from, to) in edges)
                {
                    sb.AppendLine($"{SanitizePlantUMLId(from)} --> {SanitizePlantUMLId(to)}");
                }
                
                sb.AppendLine("@enduml");
                
                WriteToFile(path, sb.ToString());
            }
            
            private string SanitizePlantUMLId(string id)
            {
                return id.Replace('-', '_');
            }
            
            private string GetPlantUMLStereotype(TargetInfo info)
            {
                return info.Target.GetTargetType() switch
                {
                    TargetType.Executable => "<<executable>>",
                    TargetType.Dynamic => "<<dynamic>>",
                    _ => "<<static>>"
                };
            }
        }
        
        #endregion
        
        #region Statistics
        
        private static void GenerateStatisticsReport(Dictionary<string, TargetInfo> targets, HashSet<(string from, string to)> edges, string path)
        {
            var sb = new StringBuilder();
            sb.AppendLine("=== Dependency Graph Statistics ===");
            sb.AppendLine($"Generated: {DateTime.Now:yyyy-MM-dd HH:mm:ss}");
            sb.AppendLine();
            
            sb.AppendLine($"Total Targets: {targets.Count}");
            sb.AppendLine($"Total Dependencies: {edges.Count}");
            sb.AppendLine();
            
            // Target type breakdown
            sb.AppendLine("Target Types:");
            var typeGroups = targets.GroupBy(t => t.Value.Target.GetTargetType());
            foreach (var group in typeGroups)
            {
                sb.AppendLine($"  {group.Key}: {group.Count()}");
            }
            sb.AppendLine();
            
            // Most dependencies
            sb.AppendLine("Top 10 Targets by Direct Dependencies:");
            var topDeps = targets.OrderByDescending(t => t.Value.DirectDependencyCount).Take(10);
            foreach (var (name, info) in topDeps)
            {
                sb.AppendLine($"  {name}: {info.DirectDependencyCount} dependencies");
            }
            sb.AppendLine();
            
            // Most depended upon
            sb.AppendLine("Top 10 Most Depended Upon Targets:");
            var dependedUpon = edges.GroupBy(e => e.to).OrderByDescending(g => g.Count()).Take(10);
            foreach (var group in dependedUpon)
            {
                sb.AppendLine($"  {group.Key}: {group.Count()} dependents");
            }
            sb.AppendLine();
            
            // Circular dependencies
            var cycles = FindCircularDependencies(targets.Keys, edges);
            if (cycles.Any())
            {
                sb.AppendLine("WARNING: Circular Dependencies Detected:");
                foreach (var cycle in cycles)
                {
                    sb.AppendLine($"  {string.Join(" -> ", cycle)} -> {cycle.First()}");
                }
            }
            else
            {
                sb.AppendLine("No circular dependencies detected ✓");
            }
            
            File.WriteAllText(path, sb.ToString());
            Log.Information($"Generated statistics report: {path}");
        }
        
        private static void CalculateTransitiveDependencies()
        {
            foreach (var (name, info) in targetInfos)
            {
                var visited = new HashSet<string>();
                var count = CountTransitiveDependencies(name, visited, 0);
                info.TransitiveDependencyCount = count;
            }
        }
        
        private static int CountTransitiveDependencies(string target, HashSet<string> visited, int depth)
        {
            if (MaxDepth >= 0 && depth > MaxDepth) return 0;
            if (!visited.Add(target)) return 0;
            
            var count = 0;
            var deps = edges.Where(e => e.from == target).Select(e => e.to);
            
            foreach (var dep in deps)
            {
                count++;
                if (ShowTransitiveDeps)
                {
                    count += CountTransitiveDependencies(dep, visited, depth + 1);
                }
            }
            
            return count;
        }
        
        private static List<List<string>> FindCircularDependencies(IEnumerable<string> targets, HashSet<(string from, string to)> edges)
        {
            var cycles = new List<List<string>>();
            var visited = new HashSet<string>();
            var recursionStack = new HashSet<string>();
            var path = new List<string>();
            
            foreach (var target in targets)
            {
                if (!visited.Contains(target))
                {
                    FindCyclesUtil(target, visited, recursionStack, path, cycles, edges);
                }
            }
            
            return cycles;
        }
        
        private static void FindCyclesUtil(string node, HashSet<string> visited, HashSet<string> recursionStack, 
                                   List<string> path, List<List<string>> cycles, HashSet<(string from, string to)> edges)
        {
            visited.Add(node);
            recursionStack.Add(node);
            path.Add(node);
            
            var neighbors = edges.Where(e => e.from == node).Select(e => e.to);
            foreach (var neighbor in neighbors)
            {
                if (!visited.Contains(neighbor))
                {
                    FindCyclesUtil(neighbor, visited, recursionStack, path, cycles, edges);
                }
                else if (recursionStack.Contains(neighbor))
                {
                    // Found a cycle
                    var cycleStart = path.IndexOf(neighbor);
                    if (cycleStart >= 0)
                    {
                        cycles.Add(path.Skip(cycleStart).ToList());
                    }
                }
            }
            
            path.RemoveAt(path.Count - 1);
            recursionStack.Remove(node);
        }
        
        #endregion
        
        #region Style Helpers
        
        private static string GetNodeColor(TargetInfo info)
        {
            return ColorScheme switch
            {
                NodeColorScheme.ByType => GetColorByType(info.Target.GetTargetType()),
                NodeColorScheme.ByModule => GetColorByModule(info.ModulePath),
                NodeColorScheme.BySize => GetColorBySize(info.CodeSize),
                NodeColorScheme.ByDependencyCount => GetColorByDependencyCount(info.DirectDependencyCount),
                _ => "lightgray"
            };
        }
        
        private static string GetColorByType(TargetType? type)
        {
            return type switch
            {
                TargetType.Static => "lightblue",
                TargetType.Dynamic => "lightgreen",
                TargetType.Executable => "lightyellow",
                _ => "lightgray"
            };
        }
        
        private static string GetColorByModule(string modulePath)
        {
            if (modulePath.Contains("core")) return "lightcoral";
            if (modulePath.Contains("engine")) return "lightblue";
            if (modulePath.Contains("game")) return "lightgreen";
            if (modulePath.Contains("tools")) return "lightyellow";
            return "lightgray";
        }
        
        private static string GetColorBySize(long size)
        {
            if (size < 10000) return "#e6f3ff";
            if (size < 50000) return "#cce6ff";
            if (size < 100000) return "#99ccff";
            if (size < 500000) return "#66b2ff";
            return "#3399ff";
        }
        
        private static string GetColorByDependencyCount(int count)
        {
            if (count == 0) return "#f0f0f0";
            if (count < 3) return "#ffe6e6";
            if (count < 5) return "#ffcccc";
            if (count < 10) return "#ffb3b3";
            return "#ff9999";
        }
        
        private static string GetNodeShape(TargetInfo info)
        {
            return info.Target.GetTargetType() switch
            {
                TargetType.Executable => "box3d",
                TargetType.Dynamic => "component",
                _ => "box"
            };
        }
        
        #endregion
        
        #region Helper Methods
        
        private static string GetModulePath(Target target)
        {
            var rootDir = Environment.CurrentDirectory;
            var relativePath = Path.GetRelativePath(rootDir, target.Directory);
            return relativePath.Replace('\\', '/');
        }
        
        private static long EstimateCodeSize(Target target)
        {
            return CountSourceFiles(target) * 5000; // Rough estimate: 5KB per source file
        }
        
        private static int CountSourceFiles(Target target)
        {
            int count = 0;
            foreach (var fileList in target.FileLists)
            {
                count += fileList.Files.Count(f => 
                    f.EndsWith(".cpp", StringComparison.OrdinalIgnoreCase) ||
                    f.EndsWith(".c", StringComparison.OrdinalIgnoreCase) ||
                    f.EndsWith(".cc", StringComparison.OrdinalIgnoreCase) ||
                    f.EndsWith(".cxx", StringComparison.OrdinalIgnoreCase));
            }
            return count;
        }
        
        private static List<string> GetExternalDependencies(Target target)
        {
            var external = new List<string>();
            
            if (target?.Arguments != null && 
                target.Arguments.TryGetValue("Libraries", out var libs) && 
                libs is List<string> libList)
            {
                external.AddRange(libList.Where(lib => !lib.StartsWith("skr")));
            }
            
            return external;
        }
        
        private static string FormatSize(long bytes)
        {
            string[] sizes = { "B", "KB", "MB", "GB" };
            int order = 0;
            double size = bytes;
            
            while (size >= 1024 && order < sizes.Length - 1)
            {
                order++;
                size /= 1024;
            }
            
            return $"{size:0.##} {sizes[order]}";
        }
        
        #endregion
    }
}