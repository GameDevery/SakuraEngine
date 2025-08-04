#pragma once
#include <SkrV8/v8_fwd.hpp>
#include <SkrRTTR/script/scriptble_object.hpp>

// v8 includes
#include <v8-persistent-handle.h>
#include <v8-template.h>

namespace skr
{
struct V8BindProxy {
    virtual ~V8BindProxy()
    {
        if (_parent)
        {
            _parent->_children.remove(address);
        }
    }

    // validate
    inline bool is_valid() const
    {
        return _is_valid;
    }
    virtual void invalidate()
    {
        _is_valid = false;

        // invalidate children
        for (auto& [_, child] : _children)
        {
            child->invalidate();
        }
    }

    // owner ship
    inline V8BindProxy* parent() const { return _parent; }
    inline void         set_parent(V8BindProxy* parent)
    {
        if (_parent) { _parent->_children.remove(address); }
        _parent = parent;
        if (_parent) { _parent->_children.add(address, this); }
    }

public:
    void* address = nullptr;

protected:
    // validate
    bool _is_valid = true;

    // owner ship
    V8BindProxy*             _parent   = nullptr;
    Map<void*, V8BindProxy*> _children = {}; // fields
};
struct V8BPRecord : V8BindProxy {
    // info
    const RTTRType* rttr_type = nullptr;

    // v8 info
    v8::Persistent<::v8::Object> v8_object = {};
    V8Isolate*                   isolate   = nullptr;
};
struct V8BPValue : V8BPRecord {
    bool             need_delete = false;
    const V8BTValue* bind_tp     = nullptr;
};
struct V8BPObject : V8BPRecord {
    const V8BTObject* bind_tp = nullptr;
    ScriptbleObject*  object  = nullptr;
};

} // namespace skr