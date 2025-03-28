#pragma once
#include "SkrContainersDef/string.hpp"
#include "yyjson/yyjson.h"

struct CStringBuilder {
    CStringBuilder(yyjson_mut_doc* doc, skr::StringView sv)
    {
        auto val = yyjson_mut_strncpy(doc, (const char*)sv.data(), sv.size());
        cstr_    = yyjson_mut_get_str(val);
    }

    CStringBuilder(yyjson_doc* doc, skr::StringView sv)
    {
        localString = sv;
        cstr_       = localString.c_str_raw();
    }

    inline const char* c_str() { return cstr_; }

private:
    skr::String localString;
    const char* cstr_ = nullptr;
};
