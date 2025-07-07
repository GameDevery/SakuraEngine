查看 @README 了解项目概述。

## 构建
目前项目使用的构建系统是采用 C# 自研的 SB，而不是 xmake。xmake 脚本虽然存在，但是已经要被弃用。

- SB 的代码目录在 tools/SB 下；
- SB 封装了 EntityFramework ，在 Dependency.cs 的类里操作数据库，提供缓存验证和增量构建等功能。尽量使用 Dependency 而不是直接用 EntityFramework 进行数据库读写。
- SB 使用 TaskEmitter 来承托面向目标的功能，例如代码编译，程序链接，等等。实现业务功能时要尽量基于 TaskEmitter；
- SB 的程序运行有一套称为 ArgumentDriver 的设计，这个设计处理输入参数为不同程序所期望的格式。即使不实际运行编译器或是连接器，也可以从 ArgumentDriver 中获得对应的参数，可以用来生成 CompileCommands 或是用来生成 VS/XCode 工程等。

## 着色器

项目使用的 Shader 语言是自研的基于 libTooling 的 C++ 着色器，它通过把 C++ AST 翻译到 Shader AST 再翻译成 HLSL 和 MSL 等语言。

- C++ AST 解析和处理的目标为 CppSLLLVM，代码在 tools/shader_compiler/LLVM 下；
- 中间层 Shader AST 的目标为 CppSLAst，剔除了 C++ 的一些冗余语法，更加容易生成代码，实现在 tools/shader/AST 下；
