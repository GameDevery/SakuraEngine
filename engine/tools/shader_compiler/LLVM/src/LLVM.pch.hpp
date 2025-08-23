#pragma once
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Path.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Expr.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#include "CppSL/AST.hpp" // IWYU pragma: keep
#include "CppSL/magic_enum/magic_enum.hpp" // IWYU pragma: keep

#include <format> // IWYU pragma: keep
#include <filesystem> // IWYU pragma: keep
#include <vector> // IWYU pragma: keep
#include <algorithm> // IWYU pragma: keep