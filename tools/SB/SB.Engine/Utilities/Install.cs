using System.Collections.Concurrent;
using System.Threading.Tasks;
using SB.Core;

namespace SB
{
    using BS = BuildSystem;
    public static class Install
    {
        public static async Task<string> Tool(string Name)
        {
            var ToolDirectory = Path.Combine(Engine.ToolDirectory, Name);
            await Engine.ConfigureNotAwareDepend.OnChanged("Install.Tool.Download", Name, "Install.Tools", async (Depend depend) =>
            {
                var ZipFile = await Download.DownloadFile(Name + GetPlatPostfix());

                lock (InstallLocks.GetOrAdd($"Install.Tool | {Name} | Copy", (Name) => new System.Threading.Lock()))
                {
                    using (Profiler.BeginZone($"Install.Tool | {Name} | Copy", color: (uint)Profiler.ColorType.Pink1))
                    {
                        Directory.CreateDirectory(ToolDirectory);
                        if (!OperatingSystem.IsWindows())
                        {
                            foreach (var F in Directory.GetFiles(ToolDirectory, "*"))
                                System.IO.File.SetUnixFileMode(F, UnixFileMode.UserExecute | UnixFileMode.UserRead | UnixFileMode.UserWrite);
                        }
                        System.IO.Compression.ZipFile.ExtractToDirectory(ZipFile, ToolDirectory, true);

                        depend.ExternalFiles.Add(ZipFile);
                        depend.ExternalFiles.AddRange(Directory.GetFiles(ToolDirectory, "*", SearchOption.AllDirectories));
                    }
                }
            }, null, null);
            return ToolDirectory;
        }

        public static async Task SDK(string Name, Dictionary<string, string>? DirectoryMappings = null)
        {
            var IntermediateDirectory = Path.Combine(Engine.DownloadDirectory, "SDKs", Name);

            await Engine.ConfigureNotAwareDepend.OnChanged("Install.SDK.Download", Name, "Install.SDKs", async (Depend depend) =>
            {
                Directory.CreateDirectory(IntermediateDirectory);
                var ZipFile = await Download.DownloadFile(Name + GetPlatPostfix());
                using (Profiler.BeginZone($"Install.SDKs | {Name} | Download", color: (uint)Profiler.ColorType.Pink1))
                {
                    if (!OperatingSystem.IsWindows())
                    {
                        foreach (var F in Directory.GetFiles(IntermediateDirectory, "*"))
                            System.IO.File.SetUnixFileMode(F, UnixFileMode.UserExecute | UnixFileMode.UserRead | UnixFileMode.UserWrite);
                    }
                    System.IO.Compression.ZipFile.ExtractToDirectory(ZipFile, IntermediateDirectory, true);
                    depend.ExternalFiles.Add(ZipFile);
                    depend.ExternalFiles.AddRange(Directory.GetFiles(IntermediateDirectory, "*", SearchOption.AllDirectories));
                }
            }, null, null);
            
            lock (InstallLocks.GetOrAdd($"Install.SDKs | {Name} | Copy", (Name) => new System.Threading.Lock()))
            {
                using (Profiler.BeginZone($"Install.SDKs | {Name} | Copy", color: (uint)Profiler.ColorType.Pink1))
                {
                    Engine.ConfigureAwareDepend.OnChanged("Install.SDK.Copy", Name, "Install.SDKs", (Depend depend) =>
                    {
                        var BuildDirectory = Path.Combine(BS.BuildPath, $"{BS.TargetOS}-{BS.TargetArch}-{BS.GlobalConfiguration}");
                        Directory.CreateDirectory(BuildDirectory);

                        depend.ExternalFiles.AddRange(Directory.GetFiles(IntermediateDirectory, "*", SearchOption.AllDirectories));
                        if (DirectoryMappings is null)
                        {
                            depend.ExternalFiles.AddRange(DirectoryCopy(IntermediateDirectory, BuildDirectory, true));
                        }
                        else
                        {
                            foreach (var Mapping in DirectoryMappings)
                            {
                                var Source = Path.Combine(IntermediateDirectory, Mapping.Key);
                                string Destination;
                                if (Path.IsPathFullyQualified(Mapping.Value))
                                    Destination = Mapping.Value;
                                else
                                    Destination = Path.Combine(BuildDirectory, Mapping.Value);
                                depend.ExternalFiles.AddRange(DirectoryCopy(Source, Destination, true));
                            }
                        }
                    }, null, new string[] { BS.GlobalConfiguration });
                }
            }
        }

        public static async Task<string> File(string Name, string Destination)
        {
            // Download the file
            var downloadedFile = await Download.DownloadFile(Name);

            // Determine the final destination
            string finalDestination;
            finalDestination = Path.IsPathFullyQualified(Destination) ? Destination : Path.Combine(BS.BuildPath, Destination);
            finalDestination = Path.Combine(finalDestination, Path.GetFileName(downloadedFile));
            
            // Use dependency system to track the copy operation
            Engine.ConfigureAwareDepend.OnChanged("Install.File.Copy", Name, "Install.Files", (Depend depend) =>
            {
                // Ensure destination directory exists
                var destDir = Path.GetDirectoryName(finalDestination);
                if (!string.IsNullOrEmpty(destDir))
                {
                    Directory.CreateDirectory(destDir);
                }
                
                // Copy the file
                System.IO.File.Copy(downloadedFile, finalDestination, overwrite: true);
                
                // Set executable permissions on non-Windows platforms
                if (!OperatingSystem.IsWindows())
                {
                    System.IO.File.SetUnixFileMode(finalDestination, UnixFileMode.UserExecute | UnixFileMode.UserRead | UnixFileMode.UserWrite);
                }
                
                depend.ExternalFiles.Add(downloadedFile);
                depend.ExternalFiles.Add(finalDestination);
            }, new[] { downloadedFile }, new[] { BS.GlobalConfiguration });
            
            return finalDestination;
        }

        private static ConcurrentDictionary<string, System.Threading.Lock> InstallLocks = new();

        private static string GetPlatPostfix()
        {
            string plat = "";
            switch (BS.HostOS)
            {
                case OSPlatform.Windows:
                    plat = "windows";
                    break;
                case OSPlatform.Linux:
                    plat = "linux";
                    break;
                case OSPlatform.OSX:
                    plat = "macosx";
                    break;
            }
            string arch = "";
            switch (BS.HostArch)
            {
                case Architecture.X64:
                    arch = BS.HostOS == OSPlatform.OSX ? "x86_64" : "x64";
                    break;
                case Architecture.X86:
                    arch = "x86";
                    break;
                case Architecture.ARM64:
                    arch = "arm64";
                    break;
            }
            return $"-{plat}-{arch}.zip";
        }

        private static List<string> DirectoryCopy(string sourceDirName, string destDirName, bool copySubDirs)
        {
            List<string> CopiedFiles = new List<string>();

            // Get the subdirectories for the specified directory.
            DirectoryInfo dir = new DirectoryInfo(sourceDirName);

            if (!dir.Exists)
            {
                throw new DirectoryNotFoundException(
                    "Source directory does not exist or could not be found: "
                    + sourceDirName);
            }

            DirectoryInfo[] dirs = dir.GetDirectories();
            // If the destination directory doesn't exist, create it.
            if (!Directory.Exists(destDirName))
            {
                Directory.CreateDirectory(destDirName);
            }

            // Get the files in the directory and copy them to the new location.
            FileInfo[] files = dir.GetFiles();
            foreach (FileInfo file in files)
            {
                string temppath = Path.Combine(destDirName, file.Name);
                file.CopyTo(temppath, true);
                CopiedFiles.Add(temppath);
            }

            // If copying subdirectories, copy them and their contents to new location.
            if (copySubDirs)
            {
                foreach (DirectoryInfo subdir in dirs)
                {
                    string temppath = Path.Combine(destDirName, subdir.Name);
                    DirectoryCopy(subdir.FullName, temppath, copySubDirs);
                }
            }

            return CopiedFiles;
        }
    }
}