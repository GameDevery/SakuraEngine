using System.Text;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Collections.Concurrent;

namespace SB
{
    public static partial class BuildSystem
    {
        public static string GetUniqueTempFileName(string File, string Hint, string Extension, IEnumerable<string>? Args = null)
        {
            string FullIdentifier = File + (Args is null ? "" : String.Join("", Args));
            var SHA = SHA256.HashData(Encoding.UTF8.GetBytes(FullIdentifier));
            return $"{Hint}.{Path.GetFileName(File)}.{Convert.ToHexString(SHA)}.{Extension}";
        }

        public static string GetStorePath(this Target Target, string StoreName) => Directory.CreateDirectory(Path.Combine(Target.GetTempBasePath(), StoreName, Target.Name)).FullName;
        public static string GetBuildPath(this Target Target) => Directory.CreateDirectory(Path.Combine(Target.GetBuildBasePath(), $"{BuildSystem.TargetOS}/{BuildSystem.TargetArch}/")).FullName;
        public static string GetTempBasePath(this Target Target) => Target.IsFromPackage ? PackageTempPath : TempPath;
        public static string GetBuildBasePath(this Target Target) => Target.IsFromPackage ? PackageBuildPath : BuildPath;

        public static bool CachedFileExists(string Path, out DateTime dateTime)
        {
            if (cachedFileExists.TryGetValue(Path, out dateTime))
                return true;
            if (File.Exists(Path))
            {
                dateTime = File.GetLastWriteTimeUtc(Path);
                cachedFileExists[Path] = dateTime;
                return true;
            }
            return false;
        }

        private static ConcurrentDictionary<string, DateTime> cachedFileExists = new();
        public static string DepsStore = ".deps";
        public static string ObjsStore = ".objs";
        public static string TempPath { get; set; }
        public static string BuildPath { get; set; }
        public static string PackageTempPath { get; set; }
        public static string PackageBuildPath { get; set; }
    }
}