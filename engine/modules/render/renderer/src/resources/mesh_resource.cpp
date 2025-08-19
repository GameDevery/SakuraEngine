#include "SkrOS/thread.h"
#include "SkrCore/memory/sp.hpp"
#include "SkrGraphics/cgpux.hpp"
#include "SkrRT/io/vram_io.hpp"
#include "SkrRT/resource/resource_system.h"
#include "SkrRT/resource/resource_factory.h"
#include "SkrRenderer/render_mesh.h"
#include "SkrRenderer/render_device.h"
#include "SkrRenderer/resources/mesh_resource.h"

#include "SkrProfile/profile.h"

using namespace skr;

static struct SkrMeshResourceUtil
{
    struct RegisteredVertexLayout : public CGPUVertexLayout
    {
        RegisteredVertexLayout(const CGPUVertexLayout& layout, VertexLayoutId id, const char8_t* name)
            : CGPUVertexLayout(layout)
            , id(id)
            , name(name)
        {
            hash = cgpux::hash<CGPUVertexLayout>()(layout);
        }
        VertexLayoutId id;
        skr::String name;
        uint64_t hash;
    };

    using VertexLayoutIdMap = skr::FlatHashMap<VertexLayoutId, skr::SP<RegisteredVertexLayout>, skr::Hash<skr_guid_t>>;
    using VertexLayoutHashMap = skr::FlatHashMap<uint64_t, RegisteredVertexLayout*>;

    SkrMeshResourceUtil()
    {
        skr_init_mutex_recursive(&vertex_layouts_mutex_);
    }

    ~SkrMeshResourceUtil()
    {
        skr_destroy_mutex(&vertex_layouts_mutex_);
    }

    inline static VertexLayoutId AddVertexLayout(VertexLayoutId id, const char8_t* name, const CGPUVertexLayout& layout)
    {
        SMutexLock lock(vertex_layouts_mutex_);

        auto pLayout = skr::SP<RegisteredVertexLayout>::New(layout, id, name);
        if (id_map.find(id) == id_map.end())
        {
            id_map.emplace(id, pLayout);
            hash_map.emplace((uint64_t)pLayout->hash, pLayout.get());
        }
        else
        {
            // id repeated
            SKR_UNREACHABLE_CODE();
        }
        return id;
    }

    inline static CGPUVertexLayout* HasVertexLayout(const CGPUVertexLayout& layout, VertexLayoutId* outGuid = nullptr)
    {
        SMutexLock lock(vertex_layouts_mutex_);

        auto hash = cgpux::hash<CGPUVertexLayout>()(layout);
        auto iter = hash_map.find(hash);
        if (iter == hash_map.end())
        {
            return nullptr;
        }
        if (outGuid) *outGuid = iter->second->id;
        return iter->second;
    }

    inline static const char* GetVertexLayoutName(CGPUVertexLayout* pLayout)
    {
        SMutexLock lock(vertex_layouts_mutex_);

        auto hash = cgpux::hash<CGPUVertexLayout>()(*pLayout);
        auto iter = hash_map.find(hash);
        if (iter == hash_map.end())
        {
            return nullptr;
        }
        return iter->second->name.c_str_raw();
    }

    inline static VertexLayoutId GetVertexLayoutId(CGPUVertexLayout* pLayout)
    {
        SMutexLock lock(vertex_layouts_mutex_);

        auto hash = cgpux::hash<CGPUVertexLayout>()(*pLayout);
        auto iter = hash_map.find(hash);
        if (iter == hash_map.end())
        {
            return {};
        }
        return iter->second->id;
    }

    inline static const char* GetVertexLayout(VertexLayoutId id, CGPUVertexLayout* layout = nullptr)
    {
        SMutexLock lock(vertex_layouts_mutex_);

        auto iter = id_map.find(id);
        if (iter == id_map.end()) return nullptr;
        if (layout) *layout = *iter->second;
        return iter->second->name.c_str_raw();
    }

    static VertexLayoutIdMap id_map;
    static VertexLayoutHashMap hash_map;
    static SMutex vertex_layouts_mutex_;
} mesh_resource_util;
SkrMeshResourceUtil::VertexLayoutIdMap SkrMeshResourceUtil::id_map;
SkrMeshResourceUtil::VertexLayoutHashMap SkrMeshResourceUtil::hash_map;
SMutex SkrMeshResourceUtil::vertex_layouts_mutex_;

void skr_mesh_resource_free(MeshResource* mesh_resource)
{
    SkrDelete(mesh_resource);
}

void skr_mesh_resource_register_vertex_layout(VertexLayoutId id, const char8_t* name, const struct CGPUVertexLayout* in_vertex_layout)
{
    if (auto layout = mesh_resource_util.GetVertexLayout(id))
        return; // existed
    else if (auto layout = mesh_resource_util.HasVertexLayout(*in_vertex_layout))
        return; // existed
    else // do register
    {
        auto result = mesh_resource_util.AddVertexLayout(id, name, *in_vertex_layout);
        SKR_ASSERT(result == id);
    }
}

const char* skr_mesh_resource_query_vertex_layout(VertexLayoutId id, struct CGPUVertexLayout* out_vertex_layout)
{
    if (auto name = mesh_resource_util.GetVertexLayout(id, out_vertex_layout))
        return name;
    else
        return nullptr;
}

namespace skr
{
using namespace skr::resource;

MeshResource::~MeshResource() SKR_NOEXCEPT
{
}

// 1.deserialize mesh resource
// 2.install indices/vertices to GPU
// 3?.update LOD information during runtime
struct SKR_RENDERER_API MeshFactoryImpl : public MeshFactory
{
    MeshFactoryImpl(const MeshFactory::Root& root)
        : root(root)
    {
        dstorage_root = skr::String::From(root.dstorage_root);
        this->root.dstorage_root = dstorage_root.c_str();
    }

    ~MeshFactoryImpl() noexcept = default;
    skr_guid_t GetResourceType() override;
    bool AsyncIO() override { return true; }
    bool Unload(SResourceRecord* record) override;
    ESkrInstallStatus Install(SResourceRecord* record) override;
    bool Uninstall(SResourceRecord* record) override;
    ESkrInstallStatus UpdateInstall(SResourceRecord* record) override;

    enum class ECompressMethod : uint32_t
    {
        NONE,
        ZLIB,
        COUNT
    };

    struct InstallType
    {
        ECompressMethod compress_method;
    };

    struct BufferRequest
    {
        BufferRequest() SKR_NOEXCEPT = default;
        ~BufferRequest() SKR_NOEXCEPT = default;

        skr::Vector<std::string> absPaths;
        skr::Vector<skr_io_future_t> dFutures;
        skr::Vector<skr::io::VRAMIOBufferId> dBuffers;
    };

    struct UploadRequest
    {
        UploadRequest() SKR_NOEXCEPT = default;
        UploadRequest(MeshFactoryImpl* factory, MeshResource* mesh_resource) SKR_NOEXCEPT
            : factory(factory),
              mesh_resource(mesh_resource)
        {
        }
        ~UploadRequest() SKR_NOEXCEPT = default;

        MeshFactoryImpl* factory = nullptr;
        MeshResource* mesh_resource = nullptr;
        skr::Vector<std::string> resource_uris;
        skr::Vector<skr_io_future_t> ram_futures;
        skr::Vector<skr::BlobId> blobs;
        skr::Vector<skr_io_future_t> vram_futures;
        skr::Vector<skr::io::VRAMIOBufferId> uBuffers;
    };

    ESkrInstallStatus InstallImpl(SResourceRecord* record);

    Root root;
    skr::String dstorage_root;
    skr::FlatHashMap<MeshResource*, InstallType> mInstallTypes;
    skr::FlatHashMap<MeshResource*, SP<BufferRequest>> mRequests;
};

MeshFactory* MeshFactory::Create(const Root& root)
{
    return SkrNew<MeshFactoryImpl>(root);
}

void MeshFactory::Destroy(MeshFactory* factory)
{
    SkrDelete(factory);
}

skr_guid_t MeshFactoryImpl::GetResourceType()
{
    const auto resource_type = ::skr::type_id_of<MeshResource>();
    return resource_type;
}

ESkrInstallStatus MeshFactoryImpl::Install(SResourceRecord* record)
{
    auto mesh_resource = (MeshResource*)record->resource;
    if (!mesh_resource) return ESkrInstallStatus::SKR_INSTALL_STATUS_FAILED;
    if (auto render_device = root.render_device)
    {
        if (mesh_resource->install_to_vram)
        {
            return InstallImpl(record);
        }
        else
        {
            // TODO: install to RAM only
            SKR_UNIMPLEMENTED_FUNCTION();
        }
    }
    else
    {
        SKR_UNREACHABLE_CODE();
    }
    return ESkrInstallStatus::SKR_INSTALL_STATUS_FAILED;
}

ESkrInstallStatus MeshFactoryImpl::InstallImpl(SResourceRecord* record)
{
    const auto noCompression = true;
    auto vram_service = root.vram_service;
    auto mesh_resource = (MeshResource*)record->resource;
    auto guid = record->activeRequest->GetGuid();
    if (auto render_device = root.render_device)
    {
        [[maybe_unused]] auto dsqueue = render_device->get_file_dstorage_queue();
        auto batch = vram_service->open_batch(mesh_resource->bins.size());
        if (noCompression)
        {
            auto dRequest = SP<BufferRequest>::New();
            dRequest->absPaths.resize_default(mesh_resource->bins.size());
            dRequest->dFutures.resize_zeroed(mesh_resource->bins.size());
            dRequest->dBuffers.resize_zeroed(mesh_resource->bins.size());
            InstallType installType = { ECompressMethod::NONE };
            for (auto i = 0u; i < mesh_resource->bins.size(); i++)
            {
                auto binPath = skr::format(u8"{}.buffer{}", guid, i);
                // TODO: REFACTOR THIS WITH VFS PATH
                // auto fullBinPath = skr::fs::path(root.dstorage_root) / binPath.c_str();
                // auto&& thisPath = dRequest->absPaths[i];
                const auto& thisBin = mesh_resource->bins[i];
                auto&& thisFuture = dRequest->dFutures[i];
                auto&& thisDestination = dRequest->dBuffers[i];

                CGPUBufferUsages usages = CGPU_BUFFER_USAGE_NONE;
                usages |= thisBin.used_with_index ? CGPU_BUFFER_USAGE_INDEX_BUFFER : 0;
                usages |= thisBin.used_with_vertex ? CGPU_BUFFER_USAGE_VERTEX_BUFFER : 0;

                CGPUBufferDescriptor bdesc = {};
                bdesc.usages = usages;
                bdesc.memory_usage = CGPU_MEM_USAGE_GPU_ONLY;
                bdesc.size = thisBin.byte_length;
                bdesc.name = nullptr; // TODO: set name

                auto request = vram_service->open_buffer_request();
                request->set_vfs(root.vfs);
                request->set_path(binPath.c_str());
                request->set_buffer(render_device->get_cgpu_device(), &bdesc);
                request->set_transfer_queue(render_device->get_cpy_queue());
                if (mesh_resource->install_to_ram)
                {
                    auto blob = request->pin_staging_buffer();
                    mesh_resource->bins[i].blob = blob;
                }
                auto result = batch->add_request(request, &thisFuture);
                thisDestination = result.cast_static<skr::io::IVRAMIOBuffer>();
            }
            mRequests.emplace(mesh_resource, dRequest);
            mInstallTypes.emplace(mesh_resource, installType);
        }
        else
        {
            SKR_UNIMPLEMENTED_FUNCTION();
        }
        vram_service->request(batch);
    }
    else
    {
        SKR_UNREACHABLE_CODE();
    }
    return ESkrInstallStatus::SKR_INSTALL_STATUS_INPROGRESS;
}

ESkrInstallStatus MeshFactoryImpl::UpdateInstall(SResourceRecord* record)
{
    auto mesh_resource = (MeshResource*)record->resource;
    auto dRequest = mRequests.find(mesh_resource);
    if (dRequest != mRequests.end())
    {
        bool okay = true;
        for (auto&& dRequest : dRequest->second->dFutures)
        {
            okay &= dRequest.is_ready();
        }
        auto status = okay ? ESkrInstallStatus::SKR_INSTALL_STATUS_SUCCEED : ESkrInstallStatus::SKR_INSTALL_STATUS_INPROGRESS;
        if (okay)
        {
            auto render_mesh = mesh_resource->render_mesh = SkrNew<RenderMesh>();
            // TODO: remove these requests
            const auto N = dRequest->second->dBuffers.size();
            render_mesh->buffers.resize_default(N);
            for (auto i = 0u; i < N; i++)
            {
                auto pBuffer = dRequest->second->dBuffers[i]->get_buffer();
                render_mesh->buffers[i] = pBuffer;
            }
            skr_render_mesh_initialize(render_mesh, mesh_resource);

            mRequests.erase(mesh_resource);
            mInstallTypes.erase(mesh_resource);
        }
        return status;
    }
    else
    {
        SKR_UNREACHABLE_CODE();
    }
    return ESkrInstallStatus::SKR_INSTALL_STATUS_INPROGRESS;
}

bool MeshFactoryImpl::Unload(SResourceRecord* record)
{
    auto mesh_resource = (MeshResource*)record->resource;
    SkrDelete(mesh_resource);
    return true;
}

bool MeshFactoryImpl::Uninstall(SResourceRecord* record)
{
    auto mesh_resource = (MeshResource*)record->resource;
    if (mesh_resource->install_to_ram)
    {
        mesh_resource->bins.clear();
    }
    skr_render_mesh_free(mesh_resource->render_mesh);
    mesh_resource->render_mesh = nullptr;
    return true;
}

} // namespace skr