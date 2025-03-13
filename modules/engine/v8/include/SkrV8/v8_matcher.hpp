#pragma once
#include "SkrRTTR/type_signature.hpp"
#include "SkrRTTR/type.hpp"
#include "SkrRTTR/export/export_data.hpp"
#include "v8_isolate.hpp"

namespace skr
{
enum class EConvertKind
{
    None = 0,
    Primitive,
    Box,
    Wrap,
};
struct MatchSuggestion;
struct MatchSuggestionPrimitive {
    uint32_t    size      = 0;
    uint32_t    alignment = 0;
    ::skr::GUID type_id   = {};
};
template <typename T>
struct MatchSuggestionWithField {
    const ::skr::rttr::FieldData* field;
    const ::skr::rttr::Type*      field_owner;
    T                             suggestion;
};
struct MatchSuggestionBox {
    using PrimitiveMember = MatchSuggestionWithField<MatchSuggestion>;
    using BoxMember       = MatchSuggestionWithField<MatchSuggestion>;

    ::skr::rttr::Type*      type = nullptr;
    Vector<PrimitiveMember> primitive_members;
    Vector<BoxMember>       box_members;
};
struct MatchSuggestionWrap {
    ::skr::rttr::Type* type = nullptr;
    // TODO. matches data
};
struct MatchSuggestion {
    // factory
    template <typename T>
    inline static MatchSuggestion Primitive()
    {
        MatchSuggestion result;

        // setup kind
        result.kind = EConvertKind::Primitive;
        new (&result.primitive) MatchSuggestionPrimitive();

        // setup primitive data
        result.primitive.size      = sizeof(T);
        result.primitive.alignment = alignof(T);
        result.primitive.type_id   = ::skr::rttr::type_id_of<T>();

        return result;
    }

    // ctor & dtor
    inline MatchSuggestion() {}
    inline ~MatchSuggestion()
    {
        switch (kind)
        {
        case EConvertKind::Primitive:
            primitive.~MatchSuggestionPrimitive();
            break;
        case EConvertKind::Box:
            box.~MatchSuggestionBox();
            break;
        case EConvertKind::Wrap:
            wrap.~MatchSuggestionWrap();
            break;
        default:
            break;
        }
    }

    // move & copy
    inline MatchSuggestion(const MatchSuggestion& other)
    {
        kind = other.kind;
        switch (kind)
        {
        case EConvertKind::Primitive:
            new (&primitive) MatchSuggestionPrimitive(other.primitive);
            break;
        case EConvertKind::Box:
            new (&box) MatchSuggestionBox(other.box);
            break;
        case EConvertKind::Wrap:
            new (&wrap) MatchSuggestionWrap(other.wrap);
            break;
        default:
            break;
        }
    }
    inline MatchSuggestion(MatchSuggestion&& other)
    {
        kind = other.kind;
        switch (kind)
        {
        case EConvertKind::Primitive:
            new (&primitive) MatchSuggestionPrimitive(std::move(other.primitive));
            break;
        case EConvertKind::Box:
            new (&box) MatchSuggestionBox(std::move(other.box));
            break;
        case EConvertKind::Wrap:
            new (&wrap) MatchSuggestionWrap(std::move(other.wrap));
            break;
        default:
            break;
        }
    }

    // validator
    inline bool is_empty() const { return kind == EConvertKind::None; }
    inline bool is_primitive() const { return kind == EConvertKind::Primitive; }
    inline bool is_box() const { return kind == EConvertKind::Box; }
    inline bool is_wrap() const { return kind == EConvertKind::Wrap; }

public:
    EConvertKind kind = EConvertKind::None;
    union
    {
        MatchSuggestionPrimitive primitive;
        MatchSuggestionBox       box;
        MatchSuggestionWrap      wrap;
    };
};

struct Matcher {
    //!NOTE. matcher will ignore decayed pointer modifiers, you should check it outside
    //!NOTE. because the behaviour of decayed pointer is depend on where the type is used (param/field...etc)
    MatchSuggestion match_to_native(::v8::Local<::v8::Value> v8_value, rttr::TypeSignatureView signature);
    MatchSuggestion match_to_v8(rttr::TypeSignatureView signature);

    // convert
    static ::v8::Local<::v8::Value> conv_to_v8(
        const MatchSuggestion& suggestion,
        void*                  native_data
    );
    static bool conv_to_native(
        const MatchSuggestion&   suggestion,
        void*                    native_data,
        ::v8::Local<::v8::Value> v8_value,
        bool                     is_init
    );
};
} // namespace skr