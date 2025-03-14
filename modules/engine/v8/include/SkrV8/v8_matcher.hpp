#pragma once
#include "SkrRTTR/type_signature.hpp"
#include "SkrRTTR/type.hpp"
#include "SkrRTTR/export/export_data.hpp"
#include "v8_isolate.hpp"

namespace skr
{
struct V8MatchSuggestion {
    enum class EKind
    {
        None = 0,
        Primitive,
        Box,
        Wrap,
    };
    struct Primitive {
        uint32_t          size      = 0;
        uint32_t          alignment = 0;
        rttr::DtorInvoker dtor      = nullptr;
        ::skr::GUID       type_id   = {};
    };
    template <typename T>
    struct WithField {
        const ::skr::rttr::FieldData* field;
        const ::skr::RTTRType*      field_owner;
        T                             suggestion;
    };
    struct Box {
        using PrimitiveMember = WithField<V8MatchSuggestion>;
        using BoxMember       = WithField<V8MatchSuggestion>;

        ::skr::RTTRType*      type = nullptr;
        Vector<PrimitiveMember> primitive_members;
        Vector<BoxMember>       box_members;
    };
    struct Wrap {
        ::skr::RTTRType* type = nullptr;
    };

    // factory
    template <typename T>
    inline static V8MatchSuggestion FromPrimitive()
    {
        V8MatchSuggestion result;

        // setup kind
        result._kind = EKind::Primitive;
        new (&result._primitive) Primitive();

        // setup primitive data
        result._primitive.size      = sizeof(T);
        result._primitive.alignment = alignof(T);
        result._primitive.type_id   = ::skr::type_id_of<T>();
        if constexpr (std::is_trivially_destructible_v<T>)
        {
            result._primitive.dtor = nullptr;
        }
        else
        {
            result._primitive.dtor = +[](void* p) -> void {
                reinterpret_cast<T*>(p)->~T();
            };
        }

        return result;
    }
    inline static V8MatchSuggestion FromBox(::skr::RTTRType* type)
    {
        V8MatchSuggestion result;

        // setup kind
        result._kind = EKind::Box;
        new (&result._box) Box();

        // setup box data
        result._box.type = type;

        return result;
    }
    inline static V8MatchSuggestion FromWrap(::skr::RTTRType* type)
    {
        V8MatchSuggestion result;

        // setup kind
        result._kind = EKind::Wrap;
        new (&result._wrap) Wrap();

        // setup wrap data
        result._wrap.type = type;

        return result;
    }

    // ctor & dtor
    inline V8MatchSuggestion() {}
    inline ~V8MatchSuggestion() { reset(); }

    // move & copy
    inline V8MatchSuggestion(const V8MatchSuggestion& other)
    {
        _kind = other._kind;
        switch (_kind)
        {
        case EKind::Primitive:
            new (&_primitive) Primitive(other._primitive);
            break;
        case EKind::Box:
            new (&_box) Box(other._box);
            break;
        case EKind::Wrap:
            new (&_wrap) Wrap(other._wrap);
            break;
        default:
            break;
        }
    }
    inline V8MatchSuggestion(V8MatchSuggestion&& other)
    {
        _kind = other._kind;
        switch (_kind)
        {
        case EKind::Primitive:
            new (&_primitive) Primitive(std::move(other._primitive));
            break;
        case EKind::Box:
            new (&_box) Box(std::move(other._box));
            break;
        case EKind::Wrap:
            new (&_wrap) Wrap(std::move(other._wrap));
            break;
        default:
            break;
        }
    }

    // assign & move assign
    inline V8MatchSuggestion& operator=(const V8MatchSuggestion& other)
    {
        if (this != &other)
        {
            reset();
            _kind = other._kind;
            switch (_kind)
            {
            case EKind::Primitive:
                new (&_primitive) Primitive(other._primitive);
                break;
            case EKind::Box:
                new (&_box) Box(other._box);
                break;
            case EKind::Wrap:
                new (&_wrap) Wrap(other._wrap);
                break;
            default:
                break;
            }
        }
        return *this;
    }
    inline V8MatchSuggestion& operator=(V8MatchSuggestion&& other)
    {
        if (this != &other)
        {
            reset();
            _kind = other._kind;
            switch (_kind)
            {
            case EKind::Primitive:
                new (&_primitive) Primitive(std::move(other._primitive));
                break;
            case EKind::Box:
                new (&_box) Box(std::move(other._box));
                break;
            case EKind::Wrap:
                new (&_wrap) Wrap(std::move(other._wrap));
                break;
            default:
                break;
            }
        }
        return *this;
    }

    // operators
    inline void reset()
    {
        switch (_kind)
        {
        case EKind::Primitive:
            _primitive.~Primitive();
            break;
        case EKind::Box:
            _box.~Box();
            break;
        case EKind::Wrap:
            _wrap.~Wrap();
            break;
        default:
            break;
        }
        _kind = EKind::None;
    }

    // validator
    inline bool is_empty() const { return _kind == EKind::None; }
    inline bool is_primitive() const { return _kind == EKind::Primitive; }
    inline bool is_box() const { return _kind == EKind::Box; }
    inline bool is_wrap() const { return _kind == EKind::Wrap; }

    // getter
    inline EKind            kind() const { return _kind; }
    inline const Primitive& primitive() const
    {
        SKR_ASSERT(_kind == EKind::Primitive);
        return _primitive;
    }
    inline Primitive& primitive()
    {
        SKR_ASSERT(_kind == EKind::Primitive);
        return _primitive;
    }
    inline const Box& box() const
    {
        SKR_ASSERT(_kind == EKind::Box);
        return _box;
    }
    inline Box& box()
    {
        SKR_ASSERT(_kind == EKind::Box);
        return _box;
    }
    inline const Wrap& wrap() const
    {
        SKR_ASSERT(_kind == EKind::Wrap);
        return _wrap;
    }
    inline Wrap& wrap()
    {
        SKR_ASSERT(_kind == EKind::Wrap);
        return _wrap;
    }

private:
    EKind _kind = EKind::None;
    union
    {
        Primitive _primitive;
        Box       _box;
        Wrap      _wrap;
    };
};

// TODO. FunctionMatchSuggestion
// TODO. FieldMatchSuggestion

struct SKR_V8_API V8Matcher {
    // basic match
    //! NOTE. matcher will ignore decayed pointer modifiers, you should check it outside
    //! NOTE. because the behaviour of decayed pointer is depend on where the type is used (param/field...etc)
    V8MatchSuggestion match_to_native(::v8::Local<::v8::Value> v8_value, TypeSignatureView signature);
    V8MatchSuggestion match_to_v8(TypeSignatureView signature);

    // basic convert
    static ::v8::Local<::v8::Value> conv_to_v8(
        const V8MatchSuggestion& suggestion,
        void*                    native_data
    );
    static bool conv_to_native(
        const V8MatchSuggestion& suggestion,
        void*                    native_data,
        ::v8::Local<::v8::Value> v8_value,
        bool                     is_init
    );

    // function match
    template <typename Data>
    bool match_params_to_native(
        const Data*                                data,
        const v8::FunctionCallbackInfo<v8::Value>& v8_func_info,
        Vector<V8MatchSuggestion>&                 out_suggestions
    );

    // function conv
    bool call_native_push_params(
        const Vector<V8MatchSuggestion>&           suggestions,
        const Vector<rttr::ParamData*>&            params,
        rttr::DynamicStack&                        stack,
        const v8::FunctionCallbackInfo<v8::Value>& v8_func_info
    );
    v8::Local<v8::Value> call_native_read_return(
        V8MatchSuggestion&  suggestion,
        rttr::DynamicStack& stack
    );

    // field match

    // field conv

private:
    // match helper
    static V8MatchSuggestion _suggest_primitive(GUID type_id);
    V8MatchSuggestion        _suggest_box(RTTRType* type);
    V8MatchSuggestion        _suggest_wrap(RTTRType* type);
    bool                     _match_primitive(const V8MatchSuggestion& suggestion, v8::Local<v8::Value> v8_value);
    bool                     _match_box(const V8MatchSuggestion& suggestion, v8::Local<v8::Value> v8_value);
    bool                     _match_wrap(const V8MatchSuggestion& suggestion, v8::Local<v8::Value> v8_value);

    // convert helpers
    static ::v8::Local<::v8::Value> _to_v8_primitive(
        const V8MatchSuggestion& suggestion,
        void*                    native_data
    );
    static bool _to_native_primitive(
        const V8MatchSuggestion& suggestion,
        ::v8::Local<::v8::Value> v8_value,
        void*                    native_data,
        bool                     is_init
    );
    static ::v8::Local<::v8::Value> _to_v8_box(
        const V8MatchSuggestion& suggestion,
        void*                    native_data
    );
    static bool _to_native_box(
        const V8MatchSuggestion& suggestion,
        ::v8::Local<::v8::Value> v8_value,
        void*                    native_data,
        bool                     is_init
    );
    static ::v8::Local<::v8::Value> _to_v8_wrap(
        const V8MatchSuggestion& suggestion,
        void*                    native_data
    );
    static bool _to_native_wrap(
        const V8MatchSuggestion& suggestion,
        ::v8::Local<::v8::Value> v8_value,
        void*                    native_data,
        bool                     is_init
    );
};
} // namespace skr

// inline impl
namespace skr
{
template <typename Data>
inline bool V8Matcher::match_params_to_native(
    const Data*                                data,
    const v8::FunctionCallbackInfo<v8::Value>& v8_func_info,
    Vector<V8MatchSuggestion>&                 out_suggestions
)
{
    using namespace ::v8;

    auto call_length   = v8_func_info.Length();
    auto native_length = data->param_data.size();

    // match length
    if (call_length != native_length)
    {
        return false;
    }

    // match params
    for (uint32_t i = 0; i < call_length; ++i)
    {
        auto                    call_value       = v8_func_info[i];
        TypeSignatureView native_signature = data->param_data[i].type.view();

        // get param ref info
        auto pointer_level = native_signature.decayed_pointer_level();
        if (pointer_level > 1) { return false; } // cannot support multi-level pointer

        // match suggestion
        auto suggestion = match_to_native(call_value, native_signature);
        if (suggestion.is_empty())
        {
            return false;
        }
    }

    return true;
}
} // namespace skr