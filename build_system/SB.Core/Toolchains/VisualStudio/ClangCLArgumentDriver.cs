using System.IO;

namespace SB.Core
{
    using ArgumentName = string;
    using VS = VisualStudio;
    using BS = BuildSystem;
    public class ClangCLArgumentDriver : CLArgumentDriver
    {
        public override string SourceDependencies(string path) => BS.CheckFile(path, false) ? $"/clang:-MD /clang:-MF\"{path}\"" : throw new TaskFatalError($"SourceDependencies value {path} is not a valid absolute path!");
        public override string PCHObject(string path) => BS.CheckFile(path, false) ? $"-Fp\"{path}\" -I\"{path}\"" : throw new TaskFatalError($"PCHObject value {path} is not a valid absolute path!");
    }
}