#include "CppSL/Attr.hpp"

namespace skr::CppSL {

AlignAttr::AlignAttr(uint32_t alignment) 
    : _alignment(alignment) 
{

}

BuiltinAttr::BuiltinAttr(const String& name) 
    : _name(name) 
{

}

KernelSizeAttr::KernelSizeAttr(uint32_t x, uint32_t y, uint32_t z)
    : _x(x), _y(y), _z(z) 
{

}

ResourceBindAttr::ResourceBindAttr()
    : _group(~0), _binding(~0) 
{

}

ResourceBindAttr::ResourceBindAttr(uint32_t group, uint32_t binding)
    : _group(group), _binding(binding) 
{

}

PushConstantAttr::PushConstantAttr()
{

}

StageInoutAttr::StageInoutAttr()
{

}

StageAttr::StageAttr(ShaderStage stage) 
    : _stage(stage)
{

}

} // namespace skr::CppSL