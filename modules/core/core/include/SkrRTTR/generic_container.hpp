#pragma once
#include <SkrRTTR/type.hpp>
#include <SkrCore/log.hpp>
#include <SkrContainers/optional.hpp>
#include <SkrContainers/vector.hpp>

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
struct GenericVector {
    inline GenericVector() = default;
    inline GenericVector(VectorMemoryBase* memory, const RTTRType* inner_type)
        : _memory(memory)
        , _inner_type(inner_type)
    {
    }

    // getter
    inline bool     is_valid() const { return _memory != nullptr; }
    inline uint64_t size() const { return _memory->size(); }
    inline uint64_t capacity() const { return _memory->capacity(); }
    inline void*    data() const { return _memory->_generic_only_data(); }

    // memory op
    // void clear();
    // void release(uint64_t reserve_capacity = 0);
    // void reserve(uint64_t expect_capacity);
    // void shrink();
    // void resize(uint64_t expect_size, void* new_value);
    // void resize_unsafe(uint64_t expect_size);
    // void resize_default(uint64_t expect_size);
    // void resize_zeroed(uint64_t expect_size);

    // add
    // void add(void* v, uint64_t n = 1);
    // void add_unsafe(uint64_t n = 1);
    // void add_default(uint64_t n = 1);
    // void add_zeroed(uint64_t n = 1);
    // void add_unique(void* v);

    // add at
    // void add_at(uint64_t idx, void* v, uint64_t n = 1);
    // void add_at_unsafe(uint64_t idx, uint64_t n = 1);
    // void add_at_default(uint64_t idx, uint64_t n = 1);
    // void add_at_zeroed(uint64_t idx, uint64_t n = 1);

    // append
    // void append(GenericVector* other);

    // append at
    // void append_at(uint64_t idx, GenericVector* other);

    // remove
    // void remove_at(uint64_t idx, uint64_t n = 1);
    // void remove_at_swap(uint64_t idx, uint64_t n = 1);
    // void remove(void* v);
    // void remove_swap(void* v);
    // void remove_last(void* v);
    // void remove_last_swap(void* v);
    // void remove_all(void* v);
    // void remove_all_swap(void* v);

    // access
    // void* at(uint64_t idx) const;
    // void* at_last(uint64_t idx) const;

    // find
    // uint64_t find(void* v) const;
    // uint64_t find_last(void* v) const;

    // sort
    // void sort_less();
    // void sort_greater();
    // void sort_stable_less();
    // void sort_stable_greater();

private:
    VectorMemoryBase* _memory     = nullptr;
    const RTTRType*   _inner_type = nullptr;
};
} // namespace skr