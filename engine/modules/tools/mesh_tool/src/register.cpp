#include "SkrCore/module/module.hpp"
#include "SkrMeshTool/mesh_asset.hpp"
#include "SkrRenderer/resources/mesh_resource.h"

struct MeshToolModule : public skr::IDynamicModule
{
public:
    virtual void on_load(int argc, char8_t** argv) override
    {
#define _DEFAULT_COOKER(__COOKER_TYPE, __RESOURCE_TYPE) skd::asset::RegisterCooker<__COOKER_TYPE>(true, skr::RTTRTraits<__COOKER_TYPE>::get_guid(), skr::RTTRTraits<__RESOURCE_TYPE>::get_guid());
        _DEFAULT_COOKER(skd::asset::MeshCooker, skr::MeshResource)
#undef _DEFAULT_COOKER

#define _IMPORTER(__TYPE) skd::asset::RegisterImporter<__TYPE>(skr::RTTRTraits<__TYPE>::get_guid());
        _IMPORTER(skd::asset::GltfMeshImporter)
        _IMPORTER(skd::asset::ProceduralMeshImporter)
#undef _IMPORTER
    }
    virtual void on_unload() override
    {
    }
};
IMPLEMENT_DYNAMIC_MODULE(MeshToolModule, SkrMeshTool);
