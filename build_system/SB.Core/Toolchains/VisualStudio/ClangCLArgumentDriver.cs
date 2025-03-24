using System.IO;

namespace SB.Core
{
    using ArgumentName = string;
    using VS = VisualStudio;
    using BS = BuildSystem;
    public class ClangCLArgumentDriver : CLArgumentDriver
    {
        public override string SourceDependencies(string path) => BS.CheckFile(path, false) ? $"/clang:-MD /clang:-MF\"{path}\"" : throw new TaskFatalError($"SourceDependencies value {path} is not a valid absolute path!");
        public virtual string AsPCHHeader(bool flag) => flag ? $"-x c++-header" : "";
        public override string UsePCHAST(string path) => BS.CheckFile(path, false) ? $"/clang:-include-pch /clang:\"{path}\"" : throw new TaskFatalError($"PCHObject value {path} is not a valid absolute path!");
    }
}