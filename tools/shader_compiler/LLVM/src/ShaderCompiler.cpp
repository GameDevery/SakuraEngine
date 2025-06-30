#include <optional>

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Path.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Frontend/CompilerInstance.h>

#include "SSL/ShaderCompiler.hpp"
#include "ShaderASTConsumer.hpp"

namespace skr::SSL {

using namespace clang::tooling;
static llvm::cl::OptionCategory ToolOptionsCategory = llvm::cl::OptionCategory("SSL compiler options");   

template <typename T>
std::unique_ptr<FrontendActionFactory> newFrontendActionFactory2(skr::SSL::AST& AST) 
{
  class SimpleFrontendActionFactory : public FrontendActionFactory 
  {
  public:
    SimpleFrontendActionFactory(skr::SSL::AST& AST) : AST(AST) {}

    std::unique_ptr<clang::FrontendAction> create() override {
      return std::make_unique<T>(AST);
    }
    skr::SSL::AST& AST;
  };

  return std::unique_ptr<FrontendActionFactory>(new SimpleFrontendActionFactory(AST));
}

struct ShaderCompilerImpl : public ShaderCompiler
{
public:
    ShaderCompilerImpl(int argc, const char **argv)
    {
        auto ExpectedParser = CommonOptionsParser::create(argc, argv, ToolOptionsCategory);
        if (!ExpectedParser)
        {
            llvm::errs() << ExpectedParser.takeError();
        }
        OptionsParser = std::move(ExpectedParser.get());
        tool.emplace(ClangTool(OptionsParser->getCompilations(), OptionsParser->getSourcePathList()));
    }

    int Run() override
    {
        auto factory = newFrontendActionFactory2<CompileFrontendAction>(AST);
        return tool->run(factory.get());
    }

    const AST& GetAST() const override
    {
        return AST;
    }

private:
    std::optional<CommonOptionsParser> OptionsParser;
    std::optional<ClangTool> tool;
    skr::SSL::AST AST;
};

ShaderCompiler* ShaderCompiler::Create(int argc, const char **argv)
{
    return new ShaderCompilerImpl(argc, argv);
}

void ShaderCompiler::Destroy(ShaderCompiler* compiler)
{
    delete compiler;
}

} // namespace skr::SSL