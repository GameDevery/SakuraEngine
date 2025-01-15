#pragma once
#include "SkrBase/config.h"

#ifdef __cplusplus
extern "C" {
#endif

SKR_CORE_API void* nswindow_create();
SKR_CORE_API void* nswindow_get_content_view(void*);

#ifdef __cplusplus
}
#endif