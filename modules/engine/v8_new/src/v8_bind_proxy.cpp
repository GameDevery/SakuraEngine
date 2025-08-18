#include <SkrV8/v8_bind_proxy.hpp>
#include <SkrV8/bind_template/v8bt_value.hpp>
#include <SkrV8/bind_template/v8bt_object.hpp>
#include <SkrV8/v8_isolate.hpp>

namespace skr
{
//======================V8BindProxy======================
V8BindProxy::~V8BindProxy()
{
    if (_parent)
    {
        _parent->_children.remove(address);
    }
}

// validate
void V8BindProxy::invalidate()
{
    SKR_ASSERT(_is_valid);
    _is_valid = false;

    // invalidate children
    for (auto& [_, child] : _children)
    {
        child->invalidate();
    }
}

//======================V8BPValue======================
void V8BPValue::invalidate()
{
    Super::invalidate();

    v8_object.Reset();

    // do release
    if (need_delete)
    {
        // call dtor
        bind_tp->call_dtor(address);

        // free memory
        rttr_type->free(address);
    }

    isolate->unregister_bind_proxy(address, this);
}

//======================V8BPObject======================
void V8BPObject::invalidate()
{
    Super::invalidate();

    v8_object.Reset();

    // remove mixin first, for prevent delete callback
    object->set_mixin_core(nullptr);

    // release object
    if (object->ownership() == EScriptbleObjectOwnership::Script)
    {
        SkrDelete(object);
    }

    isolate->unregister_bind_proxy(address, this);
}
} // namespace skr