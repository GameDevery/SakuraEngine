#pragma once
#include "SkrBase/meta.h"
#include "SkrRT/ecs/component.hpp"

#ifndef __meta__
    #include "AnimView/components.generated.h"
#endif


namespace animd
{

sreflect_struct(
    guid = "01982c5b-f8f3-74ab-be85-b8aad12792cc" 
    ecs.comp = @enable
    serde = @bin | @json)
DummyComponent
{
    int value = 0;
};


} // namespace animd