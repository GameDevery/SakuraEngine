using System.IO;

namespace SB.Core
{
    using ArgumentName = string;
    using VS = VisualStudio;
    using BS = BuildSystem;
    public class ClangCLArgumentDriver : CLArgumentDriver, IArgumentDriver
    {
        public ClangCLArgumentDriver(CFamily lang)
            : base(lang)
        {
            RawArguments.Add("-ftime-trace");
        }

        [TargetProperty(InheritBehavior = true)] 
        public virtual string[] CppFlags_ClangCl(ArgumentList<string> flags) => CppFlags(flags);

        [TargetProperty(InheritBehavior = true)] 
        public virtual string[] CFlags_ClangCl(ArgumentList<string> flags) => CFlags(flags);

        [TargetProperty(InheritBehavior = true)] 
        public virtual string[] CXFlags_ClangCl(ArgumentList<string> flags) => CXFlags(flags);
        
        public override string SourceDependencies(string path) => BS.CheckFile(path, false) ? $"/clang:-MD /clang:-MF\"{path}\"" : throw new TaskFatalError($"SourceDependencies value {path} is not a valid absolute path!");
        public virtual string AsPCHHeader(bool flag)
        {
            if (flag)
            {
                RawArguments.Remove("/TP");
                RawArguments.Remove("/TC");
                return "-x c++-header";
            }
            return "";
        }
        public override string UsePCHAST(string path) => BS.CheckFile(path, false) ? $"/clang:-include-pch /clang:\"{path}\"" : throw new TaskFatalError($"PCHObject value {path} is not a valid absolute path!");
        public override string DynamicDebug(bool v) => "";
    }
}