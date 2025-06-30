#pragma once
#include "Stmt.hpp"

namespace skr::SSL {

struct Attr
{
    virtual ~Attr() = default;
};

struct AlignAttr final : public Attr
{
public:
    uint32_t alignment() const { return _alignment; }

private:
    friend struct AST;
    AlignAttr(uint32_t alignment);
    uint32_t _alignment = 0;
};

struct BuiltinAttr : public Attr
{
public:
    const String& name() const { return _name; }

private:
    friend struct AST;
    BuiltinAttr(const String& name);
    String _name;
};

struct KernelSizeAttr : public Attr
{
public:
    uint32_t x() const { return _x; }
    uint32_t y() const { return _y; }
    uint32_t z() const { return _z; }

private:
    friend struct AST;
    KernelSizeAttr(uint32_t x, uint32_t y, uint32_t z);
    uint32_t _x = 1;
    uint32_t _y = 1;
    uint32_t _z = 1;
};

struct ResourceBindAttr : public Attr
{
public:
    // TODO: use group and binding
    // uint32_t group() const { return _group; }
    // uint32_t binding() const { return _binding; }

private:
    friend struct AST;
    ResourceBindAttr();
    ResourceBindAttr(uint32_t binding, uint32_t group = 0);
    uint32_t _group = 0;
    uint32_t _binding = 0;
};

struct StageAttr : public Attr
{
public:
    ShaderStage stage() const { return _stage; }

private:
    friend struct AST;
    StageAttr(ShaderStage stage);
    ShaderStage _stage;
};

} // namespace skr::SSL