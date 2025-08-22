#include "SkrBase/misc/defer.hpp"
#include "SkrCore/log.hpp"
#include "SkrRTTR/type.hpp"
#include "SkrToolCore/cook_system/cook_system.hpp"
#include "SkrToolCore/project/project.hpp"
#include "SkrMeshCore/mesh_processing.hpp"
#include "SkrMeshCore/builtin_mesh_asset.hpp"
#include "SkrMeshCore/builtin_mesh.hpp"
#include "SkrGraphics/api.h"

void* skd::asset::BuiltinMeshImporter::Import(skr::io::IRAMService* ioService, CookContext* context)
{
    skr::RTTRType* type = get_type_from_guid(built_in_mesh_tid);
    if (!type)
    {
        SKR_LOG_FATAL(u8"BuiltinMeshImporter: Failed to find type for guid %s", built_in_mesh_tid);
        return nullptr;
    }
    void* data = sakura_malloc_aligned(type->size(), type->alignment());
    type->find_default_ctor().invoke(data);
    return (void*)data;
}

void skd::asset::BuiltinMeshImporter::Destroy(void* resource)
{
    skr::RTTRType* type = get_type_from_guid(built_in_mesh_tid);
    sakura_free_aligned((BuiltInMesh*)resource, type->alignment());
    return;
}

bool skd::asset::BuiltinMeshCooker::Cook(CookContext* ctx)
{
    // builtin mesh cooker still need a metadata file, because its vertexLayout and installType still need to be set
    auto assetMetaFile = ctx->GetAssetMetaFile();
    auto& mesh_asset = *assetMetaFile->GetMetadata<MeshAsset>();

    auto pBuiltInMesh = ctx->Import<BuiltInMesh>();
    if (!pBuiltInMesh)
    {
        SKR_LOG_FATAL(u8"BuiltinMeshCooker: Failed to import built-in mesh for asset %s!", ctx->GetAssetPath().c_str());
    }

    MeshResource mesh;
    skr::Vector<skr::Vector<uint8_t>> blobs;
    skr_guid_t shuffle_layout_id = mesh_asset.vertexType;
    pBuiltInMesh->generate_resource(mesh, blobs, shuffle_layout_id);
    mesh.install_to_vram = mesh_asset.install_to_vram;
    // no need to optimize built-in meshes
    //----- write resource object
    if (!ctx->Save(mesh)) return false;

    // write bins using ResourceVFS
    for (size_t i = 0; i < blobs.size(); i++)
    {
        auto filename = skr::format(u8"{}.buffer{}", assetMetaFile->GetGUID(), i);
        if (!ctx->SaveExtra(blobs[i], filename.c_str()))
        {
            SKR_LOG_FMT_ERROR(u8"[MeshCooker::Cook] failed to write buffer {} for resource {}!",
                i,
                assetMetaFile->GetGUID());
            return false;
        }
    }
    return true;
}

uint32_t skd::asset::BuiltinMeshCooker::Version()
{
    return kDevelopmentVersion;
}