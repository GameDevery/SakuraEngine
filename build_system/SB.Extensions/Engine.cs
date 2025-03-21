using SB;
using SB.Core;
using System.Runtime.CompilerServices;

namespace SB
{  
    public partial class Engine : BuildSystem
    {
        public static void SetEngineDirectory(string Directory)
        {
            EngineDirectory = Directory;
        }
        
        public static string EngineDirectory { get; private set; } = Directory.GetCurrentDirectory();
        public static string ToolDirectory => Path.Combine(EngineDirectory, ".sb/tools");
        public static string DownloadDirectory => Path.Combine(EngineDirectory, ".sb/downloads");
    }
}