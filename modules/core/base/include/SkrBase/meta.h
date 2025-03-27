#pragma once
#include "SkrBase/config.h"

// basic meta
// #define __meta__ 
#ifdef __meta__
    #define sattr(...) [[clang::annotate(SKR_MAKE_STRING(__VA_ARGS__))]]
    #define sreflect_struct(...) struct [[clang::annotate("__reflect__")]] sattr(__VA_ARGS__)
    #define sreflect_interface(...) struct [[clang::annotate("__reflect__")]] sattr(__VA_ARGS__)
    #define sreflect_enum(...) enum [[clang::annotate("__reflect__")]] sattr(__VA_ARGS__)
    #define sreflect_enum_class(...) enum class [[clang::annotate("__reflect__")]] sattr(__VA_ARGS__)
    #define sreflect_function(...) [[clang::annotate("__reflect__")]] sattr(__VA_ARGS__)
#else
    #define sattr(...)
    #define sreflect_struct(...) struct
    #define sreflect_interface(...) struct
    #define sreflect_enum(...) enum
    #define sreflect_enum_class(...) enum class
    #define sreflect_function(...)
#endif

// generate body
#ifdef __meta__
    #define SKR_GENERATE_BODY() void _zz_skr_generate_body_flag();
#else
    #define _SKR_GENERATE_BODY_NAME(__FILE, __LINE) _SKR_GENERATE_BODY_NAME_IMPL(__FILE, _, __LINE)
    #define _SKR_GENERATE_BODY_NAME_IMPL(__FILE, __SEP1, __LINE) _zz_SKR_GENERATE_BODY_##__FILE##__SEP1##__LINE
    #define SKR_GENERATE_BODY() _SKR_GENERATE_BODY_NAME(SKR_FILE_ID, __LINE__)
#endif

// param flag
#define sparam_in sattr(rttr.flags += "In")
#define sparam_out sattr(rttr.flags += "Out")
#define sparam_inout sattr(rttr.flags += ["In", "Out"])

// script api
#define sscript_visible sattr(rttr.flags += "ScriptVisible")
#define sscript_newable sattr(rttr.flags += "ScriptNewable")
#define sscript_mapping sattr(rttr.flags += "ScriptMapping")
#define sscript_wrap sattr(rttr.flags += "ScriptWrap")
#define sscript_mixin sattr(rttr.flags += "ScriptMixin")
#define sscript_getter(__NAME) sattr(rttr.attrs += `ScriptGetter(SKR_UTF8(#__NAME))`)
#define sscript_setter(__NAME) sattr(rttr.attrs += `ScriptSetter(SKR_UTF8(u8#__NAME))`)

// rttr helper
#define srttr_flag(__FLAG) sattr(rttr.flags += #__FLAG)
#define srttr_attr(...) sattr(rttr.attrs += `__VA_ARGS__`)