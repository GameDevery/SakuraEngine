using SB.Core;

namespace SB
{
    public static class Install
    {
        public static string Tool(string Name)
        {
            var ToolDirectory = Path.Combine(Engine.ToolDirectory, Name);
            Depend.OnChanged("Download", Name, "Install.Tools", (Depend depend) =>
            {
                var ZipFile = Download.DownloadFile(Name + GetToolPostfix());
                Directory.CreateDirectory(ToolDirectory);
                System.IO.Compression.ZipFile.ExtractToDirectory(ZipFile, ToolDirectory, true);

                depend.ExternalFiles.Add(ZipFile);
                depend.ExternalFiles.AddRange(Directory.GetFiles(ToolDirectory, "*", SearchOption.AllDirectories));
            }, null, null);
            return ToolDirectory;
        }

        public static void SDK(string Name)
        {
            var IntermediateDirectory = Path.Combine(Engine.DownloadDirectory, "SDKs", Name);
            Directory.CreateDirectory(IntermediateDirectory);

            Depend.OnChanged("Download", Name, "Install.SDKs", (Depend depend) =>
            {
                var ZipFile = Download.DownloadFile(Name + GetToolPostfix());
                System.IO.Compression.ZipFile.ExtractToDirectory(ZipFile, IntermediateDirectory, true);

                depend.ExternalFiles.Add(ZipFile);
                depend.ExternalFiles.AddRange(Directory.GetFiles(IntermediateDirectory, "*", SearchOption.AllDirectories));
            }, null, null);

            Depend.OnChanged("Copy", Name, "Install.SDKs", (Depend depend) =>
            {
                var BuildDirectory = Path.Combine(Engine.BuildPath, BuildSystem.TargetOS.ToString(), BuildSystem.TargetArch.ToString());
                Directory.CreateDirectory(BuildDirectory);

                depend.ExternalFiles.AddRange(Directory.GetFiles(IntermediateDirectory, "*", SearchOption.AllDirectories));
                depend.ExternalFiles.AddRange(DirectoryCopy(IntermediateDirectory, BuildDirectory, true));
            }, null, null);
        }

        private static string GetToolPostfix()
        {
            string plat = "";
            switch (BuildSystem.HostOS)
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
            switch (BuildSystem.HostArch)
            {
                case Architecture.X64:
                    arch = BuildSystem.HostOS == OSPlatform.OSX ? "x86_64" : "x64";
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