#pragma once
#include <SkrRTTR/generic/generic_base.hpp>
#include <SkrContainers/optional.hpp>

namespace skr
{
struct GenericOptional {
    inline GenericOptional() = default;
    inline GenericOptional(void* memory, const RTTRType* inner_type)
        : _memory(memory)
        , _inner_type(inner_type)
    {
        SKR_ASSERT(memory != nullptr);
        _value_ptr = reinterpret_cast<uint8_t*>(_memory) + std::max(sizeof(bool), inner_type->alignment());
    }

    // getter
    inline bool  is_valid() const { return _memory != nullptr; }
    inline bool  has_value() const { return _has_value(); }
    inline void* value_ptr() const { return _value_ptr; }

    // ctor & copy & dtor
    inline void invoke_default_ctor()
    {
        _has_value() = false;
    }
    inline void invoke_copy_ctor(GenericOptional* rhs)
    {
        if (_memory == rhs->_memory) return;
        _has_value() = rhs->_has_value();
        if (_has_value())
        {
            if (_inner_type->is_record())
            {
                _copy_ctor.invoke(_value_ptr, rhs->_value_ptr);
            }
            else
            {
                std::memcpy(_value_ptr, rhs->_value_ptr, _inner_type->size());
            }
        }
    }
    inline void invoke_dtor()
    {
        reset();
    }

    // init
    inline bool init()
    {
        // find dtor
        _dtor = _inner_type->dtor_invoker();

        bool failed = false;
        if (_inner_type->is_record())
        {
            // find ctor
            auto ctor_data = _inner_type->find_default_ctor();
            if (ctor_data)
            {
                _ctor = ctor_data;
            }
            else
            {
                failed = true;
            }

            // find copy ctor
            auto copy_ctor_data = _inner_type->find_copy_ctor();
            if (copy_ctor_data)
            {
                _copy_ctor = copy_ctor_data;
            }
            else
            {
                failed = true;
            }
        }
        return !failed;
    }

    // setter
    inline void set_default()
    {
        reset();
        _has_value() = false;
        if (_inner_type->is_record())
        {
            _ctor.invoke(_value_ptr);
        }
    }
    inline void set_value(void* v)
    {
        reset();
        _has_value() = true;
        if (_inner_type->is_record())
        {
            _copy_ctor.invoke(_value_ptr, v);
        }
    }
    inline void reset()
    {
        if (_has_value())
        {
            _has_value() = false;
            if (_dtor)
            {
                _dtor(_value_ptr);
            }
        }
    }

private:
    inline bool& _has_value() { return *reinterpret_cast<bool*>(_memory); }
    inline bool  _has_value() const { return *reinterpret_cast<bool*>(_memory); }

private:
    void*                                _memory     = nullptr;
    const RTTRType*                      _inner_type = nullptr;
    void*                                _value_ptr  = nullptr;
    DtorInvoker                          _dtor       = nullptr;
    ExportCtorInvoker<void()>            _ctor       = nullptr;
    ExportCtorInvoker<void(const void*)> _copy_ctor  = nullptr;
};
} // namespace skr