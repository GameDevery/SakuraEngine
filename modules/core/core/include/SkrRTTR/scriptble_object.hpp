#pragma once
#include "SkrRTTR/iobject.hpp"
#ifndef __meta__
    #include "SkrCore/SkrRTTR/scriptble_object.generated.h"
#endif

namespace skr
{
sreflect_enum_class(guid = "a0393643-9f1b-423a-8754-f504bab41420")
EScriptbleObjectOwnerShip {
    Script,
    Native,
};

sreflect_struct(guid = "ecb7851e-f6c5-4814-8fba-a35668a2f277")
SKR_CORE_API ScriptbleObject : virtual public skr::IObject
{
    SKR_GENERATE_BODY()
    virtual ~ScriptbleObject() = default;

    //=> ScriptbleObject API
    inline EScriptbleObjectOwnerShip script_owner_ship() const { return _owner; }
    inline void script_owner_ship_take(EScriptbleObjectOwnerShip  owner) { _owner = owner; }
    //=> ScriptbleObject API
private:
    EScriptbleObjectOwnerShip _owner = EScriptbleObjectOwnerShip::Native;
};
}