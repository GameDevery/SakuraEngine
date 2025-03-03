#pragma once
#include <SkrContainersDef/vector.hpp>

namespace skr::rttr
{
using DynamicStackDataDtor = void(void* p_stack_data);

// param pass behavior, used to tell invoker how to get param value
enum class EDynamicStackParamKind : uint8_t
{
    // param type EQUAL to input type
    //   param  |  input  |   stack   | stack.get
    // ---------|---------|-----------|-----------
    //     T    |    T    |     T     |    T&
    //     T*   |    T*   |     T*    |    T*
    //     T&   |    T*   |     T*    |    T&
    // const T& | const T*| const T*  | const T&
    //     T&&  |    T*   |     T*    |    T&
    Direct,

    // param type is POINTER/REF of input type, used to store temporal data
    // e.g. script string type -> skr::String
    //   param  |  input  |   stack   | stack.get
    // ---------|---------|-----------|-----------
    //     T    |--------[SAME AS Direct]---------
    //     T*   |    T    |     T     |    T*
    //     T&   |    T    |     T     |    T&
    // const T& |    T    |     T     | const T&
    //     T&&  |    T    |     T     |    T&
    XValue,

    // input type is POINTER/REF of param type, used to reduce copy
    //   param  |  input  |   stack   | stack.get
    // ---------|---------|-----------|-----------
    //     T    |    T*   |     T*    |    T&
    //     T*   |--------[SAME AS Direct]---------
    //     T&   |--------[SAME AS Direct]---------
    // const T& |--------[SAME AS Direct]---------
    //     T&&  |--------[SAME AS Direct]---------
    Reference,
};

// get helper
template <typename T>
struct DynamicStackParamHelper {
    void*                  data;
    EDynamicStackParamKind kind;

    inline T& get()
    {
        switch (kind)
        {
            // store T
            case EDynamicStackParamKind::Direct:
            case EDynamicStackParamKind::XValue:
                return *static_cast<T*>(data);

            // store T*
            case EDynamicStackParamKind::Reference:
                return **static_cast<T**>(data);
        }
    }
};
template <typename T>
struct DynamicStackParamHelper<T*> {
    void*                  data;
    EDynamicStackParamKind kind;

    inline T* get()
    {
        switch (kind)
        {
            // store T
            case EDynamicStackParamKind::XValue:
                return static_cast<T*>(data);

                // store T*
            case EDynamicStackParamKind::Direct:
            case EDynamicStackParamKind::Reference:
                return *static_cast<T**>(data);
        }
    }
};
template <typename T>
struct DynamicStackParamHelper<T&> {
    void*                  data;
    EDynamicStackParamKind kind;

    inline T& get()
    {
        switch (kind)
        {
            // store T
            case EDynamicStackParamKind::XValue:
                return *static_cast<T*>(data);

            // store T*
            case EDynamicStackParamKind::Direct:
            case EDynamicStackParamKind::Reference:
                return **static_cast<T**>(data);
        }
    }
};
template <typename T>
struct DynamicStackParamHelper<const T&> {
    void*                  data;
    EDynamicStackParamKind kind;

    inline const T& get()
    {
        switch (kind)
        {
            // store T
            case EDynamicStackParamKind::XValue:
                return *static_cast<T*>(data);

            // store T*
            case EDynamicStackParamKind::Direct:
            case EDynamicStackParamKind::Reference:
                return **static_cast<T**>(data);
        }
    }
};
template <typename T>
struct DynamicStackParamHelper<T&&> {
    void*                  data;
    EDynamicStackParamKind kind;

    inline T& get()
    {
        switch (kind)
        {
            // store T
            case EDynamicStackParamKind::XValue:
                return *static_cast<T*>(data);

            // store T*
            case EDynamicStackParamKind::Direct:
            case EDynamicStackParamKind::Reference:
                return **static_cast<T**>(data);
        }
    }
};

// return pass behavior, used to tell invoker how to handle return value
enum class EDynamicStackReturnKind : uint8_t
{
    // discard return value
    Discard,

    // move return value into stack
    Store,

    // move return value into given address
    StoreToRef,
};

// data
struct DynamicStackParamData {
    EDynamicStackParamKind kind   = EDynamicStackParamKind::Direct;
    uint64_t               offset = 0;
    DynamicStackDataDtor*  dtor   = nullptr;
};
struct DynamicStackReturnData {
    EDynamicStackReturnKind kind = EDynamicStackReturnKind::Discard;

    // store to ref case
    void* ref = nullptr;

    // store case
    uint64_t              offset = 0;
    DynamicStackDataDtor* dtor   = nullptr;
};

// dynamic stack
struct DynamicStack {
    static constexpr uint64_t kStackBlockSize  = 512;
    static constexpr uint64_t kStackBlockAlign = alignof(uint32_t);

    // ctor & dtor
    DynamicStack();
    ~DynamicStack();

    // move & copy
    DynamicStack(const DynamicStack&) = delete;
    DynamicStack(DynamicStack&&)      = delete;

    // assign & move assign
    DynamicStack& operator=(const DynamicStack&) = delete;
    DynamicStack& operator=(DynamicStack&&)      = delete;

    // memory op
    void clear();
    void release();

    // set param
    void* alloc_param_raw(uint64_t size, uint64_t align, EDynamicStackParamKind kind, DynamicStackDataDtor* dtor);
    template <typename T>
    T* alloc_param(EDynamicStackParamKind kind = EDynamicStackParamKind::Direct, DynamicStackDataDtor* dtor = nullptr);
    template <typename T>
    void add_param(T&& value, EDynamicStackParamKind kind = EDynamicStackParamKind::Direct, DynamicStackDataDtor* dtor = nullptr);
    template <typename T>
    void add_param(const T& value, EDynamicStackParamKind kind = EDynamicStackParamKind::Direct, DynamicStackDataDtor* dtor = nullptr);

    // get param
    EDynamicStackParamKind get_param_kind(uint64_t index);
    void*                  get_param_raw(uint64_t index);
    template <typename T>
    decltype(auto) get_param(uint64_t index);

    // set return behaviour
    void return_behaviour_discard();
    void return_behaviour_store();
    void return_behaviour_store_to_ref(void* ref);

    // store return
    bool need_store_return();
    template <typename T>
    void store_return(T&& value);

    // get return
    void* get_return_raw();
    template <typename T>
    T& get_return();

    // TODO. reserve api
    // template <typename T>
    // void reserve_param();
    // template <typename T>
    // void reserve_return();
private:
    // helper
    uint64_t _grow(uint64_t size, uint64_t align);
    uint8_t* _get_memory(uint64_t offset);
    uint8_t* _alloc_block();
    void     _free_block(uint8_t* block);

private:
    // info
    Vector<DynamicStackParamData> _param_info;
    DynamicStackReturnData        _ret_info;

    // data
    uint64_t         _used_data_top = 0;
    Vector<uint8_t*> _data_blocks;
};

// dynamic stack invoker
using MethodInvokerDynamicStack = void (*)(void* p, DynamicStack& stack);
using FuncInvokerDynamicStack   = void (*)(DynamicStack& stack);

} // namespace skr::rttr

namespace skr::rttr
{
// helper
inline uint64_t DynamicStack::_grow(uint64_t size, uint64_t align)
{
    SKR_ASSERT(size > 0 && align > 0);

    // calc next offset & top
    uint64_t real_align  = std::max(kStackBlockAlign, align);
    uint64_t next_offset = int_div_ceil(_used_data_top, real_align) * real_align;
    uint64_t next_top    = next_offset + size;

    // adjust memory block to match data block size
    if ((next_top / kStackBlockAlign) != (next_top / kStackBlockAlign))
    {
        next_offset = int_div_ceil(next_top, kStackBlockSize) * kStackBlockSize;
        next_top    = next_offset + size;
    }

    // alloc new block
    if (next_top > _data_blocks.size() * kStackBlockSize)
    {
        uint8_t* new_block = _alloc_block();
        _data_blocks.add(new_block);
    }

    // update top
    _used_data_top = next_top;

    return next_offset;
}
inline uint8_t* DynamicStack::_get_memory(uint64_t offset)
{
    SKR_ASSERT(offset < _used_data_top);

    uint64_t block_index  = offset / kStackBlockSize;
    uint64_t block_offset = offset % kStackBlockSize;

    uint8_t* block = _data_blocks.at(block_index);
    return block + block_offset;
}
inline uint8_t* DynamicStack::_alloc_block()
{
    return static_cast<uint8_t*>(sakura_malloc_aligned(kStackBlockSize, kStackBlockAlign));
}
inline void DynamicStack::_free_block(uint8_t* block)
{
    sakura_free_aligned(block, kStackBlockAlign);
}

// ctor & dtor
inline DynamicStack::DynamicStack() = default;
inline DynamicStack::~DynamicStack()
{
    release();
}

// memory op
inline void DynamicStack::clear()
{
    // destruct params
    for (const auto& info : _param_info)
    {
        if (info.dtor)
        {
            info.dtor(_get_memory(info.offset));
        }
    }
    _param_info.clear();

    // destruct return
    if (_ret_info.kind != EDynamicStackReturnKind::Discard)
    {
        if (_ret_info.dtor)
        {
            _ret_info.dtor(_get_memory(_ret_info.offset));
        }
        _ret_info = {};
    }

    // reset top
    _used_data_top = 0;
}
inline void DynamicStack::release()
{
    clear();

    for (auto block : _data_blocks)
    {
        _free_block(block);
    }
    _data_blocks.clear();
}

// set param
inline void* DynamicStack::alloc_param_raw(uint64_t size, uint64_t align, EDynamicStackParamKind kind, DynamicStackDataDtor* dtor)
{
    // alloc memory
    auto offset = _grow(size, align);

    // add param info
    auto& info  = _param_info.add_default().ref();
    info.offset = offset;
    info.kind   = kind;
    info.dtor   = dtor;

    return _get_memory(offset);
}
template <typename T>
inline T* DynamicStack::alloc_param(EDynamicStackParamKind kind, DynamicStackDataDtor* dtor)
{
    dtor = dtor ? dtor : [](void* p) { static_cast<T*>(p)->~T(); };
    return static_cast<T*>(alloc_param_raw(sizeof(T), alignof(T), kind, dtor));
}
template <typename T>
inline void DynamicStack::add_param(T&& value, EDynamicStackParamKind kind, DynamicStackDataDtor* dtor)
{
    T* p = alloc_param<T>(kind, dtor);
    new (p) T(std::move(value));
}
template <typename T>
inline void DynamicStack::add_param(const T& value, EDynamicStackParamKind kind, DynamicStackDataDtor* dtor)
{
    T* p = alloc_param<T>(kind, dtor);
    new (p) T(value);
}

// get param
inline EDynamicStackParamKind DynamicStack::get_param_kind(uint64_t index)
{
    SKR_ASSERT(index < _param_info.size());
    return _param_info.at(index).kind;
}
inline void* DynamicStack::get_param_raw(uint64_t index)
{
    SKR_ASSERT(index < _param_info.size());
    return _get_memory(_param_info.at(index).offset);
}
template <typename T>
inline decltype(auto) DynamicStack::get_param(uint64_t index)
{
    // get data
    SKR_ASSERT(index < _param_info.size());
    auto& data = _param_info.at(index);

    // fill getter
    DynamicStackParamHelper<T> getter;
    getter.data = _get_memory(data.offset);
    getter.kind = data.kind;

    return getter.get();
}

// set return behaviour
inline void DynamicStack::return_behaviour_discard()
{
    _ret_info.kind = EDynamicStackReturnKind::Discard;
}
inline void DynamicStack::return_behaviour_store()
{
    _ret_info.kind = EDynamicStackReturnKind::Store;
}
inline void DynamicStack::return_behaviour_store_to_ref(void* ref)
{
    _ret_info.kind = EDynamicStackReturnKind::StoreToRef;
    _ret_info.ref  = ref;
}

// store return
inline bool DynamicStack::need_store_return()
{
    return _ret_info.kind != EDynamicStackReturnKind::Discard;
}
template <typename T>
inline void DynamicStack::store_return(T&& value)
{
    // get pointer
    void* pointer = nullptr;
    if (_ret_info.kind == EDynamicStackReturnKind::Store)
    {
        auto offset = _grow(sizeof(T), alignof(T));
        pointer     = _get_memory(offset);
        _ret_info.offset = offset;
        _ret_info.dtor = [](void* p) { static_cast<T*>(p)->~T(); };
    }
    else if (_ret_info.kind == EDynamicStackReturnKind::StoreToRef)
    {
        pointer = _ret_info.ref;
    }

    // write to pointer
    new (pointer) T(std::forward<T>(value));
}

// get return
inline void* DynamicStack::get_return_raw()
{
    SKR_ASSERT(_ret_info.kind == EDynamicStackReturnKind::Store);
    return _get_memory(_ret_info.offset);
}
template <typename T>
inline T& DynamicStack::get_return()
{
    SKR_ASSERT(_ret_info.kind == EDynamicStackReturnKind::Store);
    return *reinterpret_cast<T*>(_get_memory(_ret_info.offset));
}
} // namespace skr::rttr