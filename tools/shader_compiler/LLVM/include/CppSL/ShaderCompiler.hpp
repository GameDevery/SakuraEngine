#pragma once

namespace skr::CppSL {
struct AST;

struct ShaderCompiler
{
    static ShaderCompiler* Create(int argc, const char **argv);
    static void Destroy(ShaderCompiler* compiler);

    virtual int Run() = 0;
    virtual const AST& GetAST() const = 0;

    virtual ~ShaderCompiler() = default;
};

}