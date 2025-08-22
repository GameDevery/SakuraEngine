#include "SkrCore/module/module.hpp"
#include "SkrMeshCore/builtin_mesh_asset.hpp"
#include "SkrRenderer/resources/mesh_resource.h"

struct SkrMeshCoreModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override
    {
        skd::asset::RegisterCooker<skd::asset::BuiltinMeshCooker>(false, skr::RTTRTraits<skd::asset::BuiltinMeshCooker>::get_guid(), skr::RTTRTraits<skr::MeshResource>::get_guid());

        skd::asset::RegisterImporter<skd::asset::BuiltinMeshImporter>(skr::RTTRTraits<skd::asset::BuiltinMeshImporter>::get_guid());
    }
    virtual void on_unload() override {}
};
IMPLEMENT_DYNAMIC_MODULE(SkrMeshCoreModule, SkrMeshCore);