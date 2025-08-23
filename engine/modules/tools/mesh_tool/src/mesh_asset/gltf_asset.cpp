#include "cgltf/cgltf.h"
#include "SkrBase/misc/defer.hpp"
#include "SkrCore/log.hpp"
#include "SkrTask/parallel_for.hpp"
#include "SkrContainers/stl_vector.hpp"
#include "SkrToolCore/cook_system/cook_system.hpp"
#include "SkrToolCore/project/project.hpp"
#include "SkrMeshTool/mesh_asset.hpp"
#include "SkrMeshTool/mesh_processing.hpp"
#include "MeshOpt/meshoptimizer.h"

#include "SkrProfile/profile.h"

namespace skd::asset
{

void* GltfMeshImporter::Import(skr::io::IRAMService* ioService, CookContext* context)
{
    const auto assetMetaFile = context->GetAssetMetaFile();
    auto path = context->AddSourceFile(assetPath);
    auto vfs = assetMetaFile->GetProject()->GetAssetVFS();
    return ImportGLTFWithData(path.string().c_str(), ioService, vfs);
}

void GltfMeshImporter::Destroy(void* resource)
{
    ::cgltf_free((cgltf_data*)resource);
}

} // namespace skd::asset
