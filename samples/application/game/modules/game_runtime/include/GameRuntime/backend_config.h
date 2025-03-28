#pragma once
#include "SkrBase/config.h"
#include "SkrRT/resource/config_resource.h"
#include "SkrContainers/function_ref.hpp"
#ifndef __meta__
    #include "GameRuntime/backend_config.generated.h" // IWYU pragma: export
#endif

sreflect_enum(
    guid = "b4b7f387-d8c2-465c-9b3a-6d83a3d198b1"
    serde = @bin|@json
)
ECGPUBackEnd SKR_ENUM(uint32_t){
    Vulkan,
    DX12,
    Metal
};
typedef enum ECGPUBackEnd ECGPUBackEnd;

sreflect_struct(
    guid = "b537f7b1-6d2d-44f6-b313-bcb559d3f490"
    serde = @bin|@json
)
config_backend_t {
    ECGPUBackEnd backend;
    ECGPUBackEnd GetBackend() const { return backend; }
    void         SetBackend(ECGPUBackEnd inBackend) { backend = inBackend; }
    void         GetBackend2(ECGPUBackEnd& outBackend) const { outBackend = backend; }
    void         GetSetBackend(ECGPUBackEnd& outBackend)
    {
        ECGPUBackEnd old = backend;
        backend          = outBackend;
        outBackend       = old;
    }
    void CopyBackend(const config_backend_t& other) { backend = other.backend; }
    void SetBackendCallback(void (*callback)(void* userdata, ECGPUBackEnd&), void* userdata) { callback(userdata, backend); }
    void GetBackendCallback(skr::FunctionRef<void(ECGPUBackEnd)> callback) const { callback(backend); }

    void* dirtyStuff() { return this; }
};
typedef struct config_backend_t config_backend_t;
