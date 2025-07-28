#include "SkrGLTFTool/mesh_asset.hpp"
#include "SkrRenderer/resources/mesh_resource.h"

struct _GLTFToolRegister {
    _GLTFToolRegister()
    {
#define _DEFAULT_COOKER(__COOKER_TYPE, __RESOURCE_TYPE) skd::asset::RegisterCooker<__COOKER_TYPE>(true, skr::RTTRTraits<__COOKER_TYPE>::get_guid(), skr::RTTRTraits<__RESOURCE_TYPE>::get_guid());
        _DEFAULT_COOKER(skd::asset::MeshCooker, skr::renderer::MeshResource)
#undef _DEFAULT_COOKER

#define _IMPORTER(__TYPE) skd::asset::RegisterImporter<__TYPE>(skr::RTTRTraits<__TYPE>::get_guid());
        _IMPORTER(skd::asset::GltfMeshImporter)
#undef _IMPORTER
    }
} _gltf_tool_register;