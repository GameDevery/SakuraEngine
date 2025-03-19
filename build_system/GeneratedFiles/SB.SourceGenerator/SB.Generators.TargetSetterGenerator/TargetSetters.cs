
using SB.Core;

namespace SB
{    
    public abstract class TargetSetters
    {

        public SB.Target Exception(bool Enable) { FinalArguments.Override("Exception", Enable); return this as SB.Target; }

        public SB.Target RuntimeLibrary(string what) { FinalArguments.Override("RuntimeLibrary", what); return this as SB.Target; }

        public SB.Target CppVersion(string what) { FinalArguments.Override("CppVersion", what); return this as SB.Target; }

        public SB.Target CVersion(string what) { FinalArguments.Override("CVersion", what); return this as SB.Target; }

        public SB.Target SIMD(global::SB.Core.SIMDArchitecture simd) { FinalArguments.Override("SIMD", simd); return this as SB.Target; }

        public SB.Target WarningLevel(global::SB.Core.WarningLevel level) { FinalArguments.Override("WarningLevel", level); return this as SB.Target; }

        public SB.Target WarningAsError(bool v) { FinalArguments.Override("WarningAsError", v); return this as SB.Target; }

        public SB.Target OptimizationLevel(global::SB.Core.OptimizationLevel opt) { FinalArguments.Override("OptimizationLevel", opt); return this as SB.Target; }

        public SB.Target FpModel(global::SB.Core.FpModel v) { FinalArguments.Override("FpModel", v); return this as SB.Target; }

        public SB.Target CppFlags(Visibility Visibility, params string[] flags) { GetArgumentsContainer(Visibility).GetOrAddNew<string, SB.Core.ArgumentList<string>>("CppFlags").AddRange(flags); return this as SB.Target; }

        public SB.Target Defines(Visibility Visibility, params string[] defines) { GetArgumentsContainer(Visibility).GetOrAddNew<string, SB.Core.ArgumentList<string>>("Defines").AddRange(defines); return this as SB.Target; }

        public SB.Target IncludeDirs(Visibility Visibility, params string[] dirs) { GetArgumentsContainer(Visibility).GetOrAddNew<string, SB.Core.ArgumentList<string>>("IncludeDirs").AddRange(dirs); return this as SB.Target; }

        public SB.Target RTTI(bool v) { FinalArguments.Override("RTTI", v); return this as SB.Target; }

        public SB.Target Source(string path) { FinalArguments.Override("Source", path); return this as SB.Target; }

        public SB.Target TargetType(global::SB.Core.TargetType type) { FinalArguments.Override("TargetType", type); return this as SB.Target; }

        public SB.Target LinkDirs(Visibility Visibility, params string[] dirs) { GetArgumentsContainer(Visibility).GetOrAddNew<string, SB.Core.ArgumentList<string>>("LinkDirs").AddRange(dirs); return this as SB.Target; }

        public SB.Target Link(Visibility Visibility, params string[] dirs) { GetArgumentsContainer(Visibility).GetOrAddNew<string, SB.Core.ArgumentList<string>>("Link").AddRange(dirs); return this as SB.Target; }

        public SB.Target WholeArchive(Visibility Visibility, params string[] libs) { GetArgumentsContainer(Visibility).GetOrAddNew<string, SB.Core.ArgumentList<string>>("WholeArchive").AddRange(libs); return this as SB.Target; }

        private Dictionary<string, object?> GetArgumentsContainer(Visibility Visibility)
        {
            switch (Visibility)
            {
                case Visibility.Public: return PublicArguments;
                case Visibility.Private: return PrivateArguments;
                case Visibility.Interface: return InterfaceArguments;
            }
            return PrivateArguments;
        }

        public IReadOnlyDictionary<string, object?> Arguments => FinalArguments;
        internal Dictionary<string, object?> FinalArguments { get; } = new();
        internal Dictionary<string, object?> PublicArguments { get; } = new();
        internal Dictionary<string, object?> PrivateArguments { get; } = new();
        internal Dictionary<string, object?> InterfaceArguments { get; } = new();
    }
}