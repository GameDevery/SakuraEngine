#include <vector>
#include <string>
#include "CppSL/ShaderCompiler.hpp"

int main(int argc, const char **argv)
{
    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i)
    {
        args.emplace_back(argv[i]);
    }
    args.emplace_back("--");
    args.emplace_back("-x");
    args.emplace_back("c++");
    args.emplace_back("-std=c++23");
    args.emplace_back("-fsyntax-only");
    args.emplace_back("-D__CPPSL__");
    // swizzle uses reference member in union
    args.emplace_back("-fms-extensions");
    args.emplace_back("-fms-compatibility-version=17.1.1");
    args.emplace_back("-Wno-microsoft-union-member-reference");

    std::vector<const char*> args_ptr(args.size());
    for (size_t i = 0; i < args.size(); ++i)
    {
        args_ptr[i] = args[i].c_str();
    }
    
    auto compiler = skr::CppSL::ShaderCompiler::Create(args_ptr.size(), args_ptr.data());
    int exit_code = 0;
    if (compiler)
    {
        exit_code = compiler->Run();
        const auto& AST = compiler->GetAST();
        skr::CppSL::ShaderCompiler::Destroy(compiler);
    }
    return exit_code;
}