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
        public virtual string[] ClangCl_CppFlags(ArgumentList<string> flags) => CppFlags(flags);

        [TargetProperty(InheritBehavior = true)] 
        public virtual string[] ClangCl_CFlags(ArgumentList<string> flags) => CFlags(flags);

        [TargetProperty(InheritBehavior = true)] 
        public virtual string[] ClangCl_CXFlags(ArgumentList<string> flags) => CXFlags(flags);

        public override string[] Cl_CppFlags(ArgumentList<string> flags) => new string[0];
        public override string[] Cl_CFlags(ArgumentList<string> flags) => new string[0];
        public override string[] Cl_CXFlags(ArgumentList<string> flags) => new string[0];

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