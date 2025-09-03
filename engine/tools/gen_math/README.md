# 如何使用
- 运行 `bun install` 安装依赖
- CLI: `bun run gen_math.ts ../engine/modules/core/base/include/SkrBase/math/gen`
- VSCode Launch:
```json
{
    "name": "[Gen] Math",
    "type": "bun",
    "request": "launch",
    "cwd": "${workspaceFolder:root}",
    "program": "${workspaceFolder:root}/tools/gen_math/gen_math.ts",
    "args": [
        "${workspaceFolder:root}/engine/modules/core/base/include/SkrBase/math/gen"
    ],
}
```