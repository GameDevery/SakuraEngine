#pragma once
#include <SkrRTTR/type.hpp>
#include <SkrCore/log.hpp>
#include <SkrContainers/optional.hpp>

// Optional
namespace skr
{
struct GenericOptional {
    inline GenericOptional(OptionalBase* optional, const RTTRType* inner_type)
        : _optional(optional)
        , _inner_type(inner_type)
    {
        _value_ptr = reinterpret_cast<uint8_t*>(_optional) + _optional->_padding;
    }

    // init
    inline bool init()
    {
        // find dtor
        _dtor = _inner_type->dtor_invoker();

        // find copy ctor
        auto copy_ctor_data = _inner_type->find_copy_ctor();
        if (copy_ctor_data)
        {
            _copy_ctor = copy_ctor_data;
            return true;
        }
        else
        {
            return false;
        }
    }

    // getter
    inline bool  has_value() const { return _optional->_has_value; }
    inline void* value_ptr() const { return _value_ptr; }

    // setter
    inline void set_value(void* v)
    {
        reset();
        _optional->_has_value = true;
        _copy_ctor.invoke(_value_ptr, v);
    }
    inline void reset()
    {
        if (_optional->_has_value)
        {
            _optional->_has_value = false;
            if (_dtor)
            {
                _dtor(_value_ptr);
            }
        }
    }

private:
    OptionalBase*                        _optional   = nullptr;
    const RTTRType*                      _inner_type = nullptr;
    void*                                _value_ptr  = nullptr;
    DtorInvoker                          _dtor       = nullptr;
    ExportCtorInvoker<void(const void*)> _copy_ctor  = nullptr;
};
} // namespace skr