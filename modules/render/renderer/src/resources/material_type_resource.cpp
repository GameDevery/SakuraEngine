#include "SkrRenderer/resources/material_type_resource.hpp"

namespace skr
{
namespace renderer
{
using namespace skr::resource;

struct SMaterialTypeFactoryImpl : public SMaterialTypeFactory {
    SMaterialTypeFactoryImpl(const SMaterialTypeFactory::Root& root)
        : root(root)
    {
    }

    skr_guid_t GetResourceType() override
    {
        return ::skr::type_id_of<skr_material_type_resource_t>();
    }
    bool AsyncIO() override { return true; }
    bool Unload(SResourceRecord* record) override
    {
        // TODO: RC management for shader collection resource
        auto material_type = static_cast<skr_material_type_resource_t*>(record->resource);
        SkrDelete(material_type);
        return true;
    }
    ESkrInstallStatus Install(SResourceRecord* record) override
    {
        auto material_type = static_cast<skr_material_type_resource_t*>(record->resource);
        return material_type ? SKR_INSTALL_STATUS_SUCCEED : SKR_INSTALL_STATUS_FAILED;
    }
    bool Uninstall(SResourceRecord* record) override
    {
        return true;
    }
    ESkrInstallStatus UpdateInstall(SResourceRecord* record) override
    {
        return SKR_INSTALL_STATUS_SUCCEED;
    }

    Root root;
};

SMaterialTypeFactory* SMaterialTypeFactory::Create(const Root& root)
{
    return SkrNew<SMaterialTypeFactoryImpl>(root);
}

void SMaterialTypeFactory::Destroy(SMaterialTypeFactory* factory)
{
    SkrDelete(factory);
}

} // namespace renderer
} // namespace skr