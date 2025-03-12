#pragma once
#include "SkrBase/config.h"

// basic meta
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
    #define SKR_GENERATE_BODY_NAME(__FILE, __LINE) SKR_GENERATE_BODY_NAME_IMPL(__FILE, _, __LINE)
    #define SKR_GENERATE_BODY_NAME_IMPL(__FILE, __SEP1, __LINE) SKR_GENERATE_BODY_##__FILE##__SEP1##__LINE
    #define SKR_GENERATE_BODY() SKR_GENERATE_BODY_NAME(SKR_FILE_ID, __LINE__)
#endif

// script api
#define sscript_visible sattr(rttr.flags += "ScriptVisible")
#define sscript_newable sattr(rttr.flags += "ScriptNewable")
#define sscript_box sattr(rttr.flags += "ScriptBox")
#define sscript_wrap sattr(rttr.flags += "ScriptWrap")