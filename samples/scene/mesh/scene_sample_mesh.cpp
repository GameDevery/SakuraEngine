#include <SkrBase/misc/make_zeroed.hpp>
#include <SkrCore/memory/sp.hpp>
#include <SkrCore/module/module_manager.hpp>
#include <SkrCore/log.h>
#include <SkrCore/platform/vfs.h>
#include <SkrCore/time.h>
#include <SkrCore/async/thread_job.hpp>
#include <SkrRT/io/vram_io.hpp>
#include "SkrOS/thread.h"
#include "SkrRT/io/ram_io.hpp"
#include "SkrRT/misc/cmd_parser.hpp"

#include <SkrOS/filesystem.hpp>
#include "SkrCore/memory/impl/skr_new_delete.hpp"
#include "SkrSystem/advanced_input.h"
#include <SkrRT/ecs/world.hpp>
#include <SkrRT/resource/resource_system.h>
#include <SkrRT/resource/local_resource_registry.hpp>

#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrImGui/imgui_app.hpp"
#include "SkrRenderer/skr_renderer.h"
#include "SkrRenderer/resources/mesh_resource.h"
#include "SkrRenderer/render_mesh.h"
#include "SkrTask/fib_task.hpp"
#include "SkrGLTFTool/mesh_processing.hpp"
#include "SkrGLTFTool/mesh_asset.hpp"
#include "SkrToolCore/project/project.hpp"
#include "SkrToolCore/cook_system/cook_system.hpp"
#include "scene_renderer.hpp"
#include "helper.hpp"
#include "cgltf/cgltf.h"
#include <cstdio>

using namespace skr::literals;
const auto MeshAssetID = u8"01985f1f-8286-773f-8bcc-7d7451b0265d"_guid;

namespace temp
{

static const skd::asset::ERawVertexStreamType kGLTFToRawAttributeTypeLUT[] = {
    skd::asset::ERawVertexStreamType::POSITION,
    skd::asset::ERawVertexStreamType::NORMAL,
    skd::asset::ERawVertexStreamType::TANGENT,
    skd::asset::ERawVertexStreamType::TEXCOORD,
    skd::asset::ERawVertexStreamType::COLOR,
    skd::asset::ERawVertexStreamType::JOINTS,
    skd::asset::ERawVertexStreamType::WEIGHTS,
    skd::asset::ERawVertexStreamType::CUSTOM,
    skd::asset::ERawVertexStreamType::Count,
};
skd::asset::SRawMesh GenerateRawMeshForGLTFMesh(cgltf_mesh* mesh)
{
    skd::asset::SRawMesh raw_mesh = {};
    raw_mesh.primitives.reserve(mesh->primitives_count);
    for (uint32_t pid = 0; pid < mesh->primitives_count; pid++)
    {
        const auto gltf_primitive = mesh->primitives + pid;
        skd::asset::SRawPrimitive& primitive = raw_mesh.primitives.add_default().ref();
        // fill indices
        {
            const auto buffer_view = gltf_primitive->indices->buffer_view;
            const auto buffer_data = static_cast<const uint8_t*>(buffer_view->data ? buffer_view->data : buffer_view->buffer->data);
            const auto view_data = buffer_data + buffer_view->offset;
            const auto indices_count = gltf_primitive->indices->count;
            primitive.index_stream.buffer_view = skr::span<const uint8_t>(view_data + gltf_primitive->indices->offset, gltf_primitive->indices->stride * indices_count);
            primitive.index_stream.offset = 0;
            primitive.index_stream.count = indices_count;
            primitive.index_stream.stride = gltf_primitive->indices->stride;
        }
        // fill vertex streams
        primitive.vertex_streams.reserve(gltf_primitive->attributes_count);
        for (uint32_t vid = 0; vid < gltf_primitive->attributes_count; vid++)
        {
            const auto& attribute = gltf_primitive->attributes[vid];
            const auto buffer_view = attribute.data->buffer_view;
            const auto buffer_data = static_cast<const uint8_t*>(buffer_view->data ? buffer_view->data : buffer_view->buffer->data);
            const auto view_data = buffer_data + buffer_view->offset;
            const auto vertex_count = attribute.data->count;
            skd::asset::SRawVertexStream& vertex_stream = primitive.vertex_streams.add_default().ref();
            vertex_stream.buffer_view = skr::span<const uint8_t>(view_data + attribute.data->offset, attribute.data->stride * vertex_count);
            vertex_stream.offset = 0;
            vertex_stream.count = vertex_count;
            vertex_stream.stride = attribute.data->stride;
            vertex_stream.type = kGLTFToRawAttributeTypeLUT[attribute.type];
        }
    }
    return raw_mesh;
}
} // namespace temp

// The Three-Triangle Example: simple mesh scene hierarchy

struct SceneSampleMeshModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;

    void InitializeReosurceSystem();
    void InitializeAssetSystem();
    void DestroyAssetSystem();
    void DestroyResourceSystem();

    void CookAndLoadGLTF();

    skr::task::scheduler_t scheduler;
    skr::ecs::World world{ scheduler };
    skr_vfs_t* resource_vfs = nullptr;
    skr::io::IRAMService* ram_service = nullptr;
    skr_io_vram_service_t* vram_service = nullptr;
    skr::SP<skr::JobQueue> job_queue = nullptr;
    SRenderDeviceId render_device = nullptr;

    skr::UPtr<skr::ImGuiApp> imgui_app = nullptr;
    skr::SceneRenderer* scene_renderer = nullptr;

    skr::String gltf_path = u8"";
    skr::resource::LocalResourceRegistry* registry = nullptr;
    skr::renderer::MeshFactory* MeshFactory = nullptr;
    bool use_gltf = false;

    skd::SProject project;

    // Currently we do not use this, but it can be useful for future extensions
    skr::renderer::MeshFactory* mesh_factory = nullptr;
};

IMPLEMENT_DYNAMIC_MODULE(SceneSampleMeshModule, SceneSample_Mesh);

void SceneSampleMeshModule::InitializeAssetSystem()
{
    auto& system = *skd::asset::GetCookSystem();
    system.Initialize();
}

void SceneSampleMeshModule::DestroyAssetSystem()
{
    auto& system = *skd::asset::GetCookSystem();
    system.Shutdown();
}

void SceneSampleMeshModule::InitializeReosurceSystem()
{
    using namespace skr::literals;
    auto resource_system = skr::resource::GetResourceSystem();
    registry = SkrNew<skr::resource::LocalResourceRegistry>(project.GetResourceVFS());
    resource_system->Initialize(registry, project.GetRamService());
    const auto resource_root = project.GetResourceVFS()->mount_dir;
    {
        skr::String qn = u8"SceneSampleMesh-JobQueue";
        auto job_queueDesc = make_zeroed<skr::JobQueueDesc>();
        job_queueDesc.thread_count = 2;
        job_queueDesc.priority = SKR_THREAD_NORMAL;
        job_queueDesc.name = qn.u8_str();
        job_queue = skr::SP<skr::JobQueue>::New(job_queueDesc);

        auto ioServiceDesc = make_zeroed<skr_ram_io_service_desc_t>();
        ioServiceDesc.name = u8"SceneSampleMesh-RAMIOService";
        ioServiceDesc.sleep_time = 1000 / 60;
        ram_service = skr_io_ram_service_t::create(&ioServiceDesc);
        ram_service->run();

        auto vramServiceDesc = make_zeroed<skr_vram_io_service_desc_t>();
        vramServiceDesc.name = u8"SceneSampleMesh-VRAMIOService";
        vramServiceDesc.awake_at_request = true;
        vramServiceDesc.ram_service = ram_service;
        vramServiceDesc.callback_job_queue = job_queue.get();
        vramServiceDesc.use_dstorage = true;
        vramServiceDesc.gpu_device = render_device->get_cgpu_device();
        vram_service = skr_io_vram_service_t::create(&vramServiceDesc);
        vram_service->run();
    }
    // mesh factory
    {
        skr::renderer::MeshFactory::Root factoryRoot = {};
        factoryRoot.dstorage_root = resource_root;
        factoryRoot.vfs = project.GetResourceVFS();
        factoryRoot.ram_service = ram_service;
        factoryRoot.vram_service = vram_service;
        factoryRoot.render_device = render_device;
        MeshFactory = skr::renderer::MeshFactory::Create(factoryRoot);
        resource_system->RegisterFactory(MeshFactory);
    }
}

void SceneSampleMeshModule::DestroyResourceSystem()
{
    auto resource_system = skr::resource::GetResourceSystem();
    resource_system->Shutdown();

    skr::renderer::MeshFactory::Destroy(MeshFactory);

    skr_io_ram_service_t::destroy(ram_service);
    skr_io_vram_service_t::destroy(vram_service);
    job_queue.reset();
}

void SceneSampleMeshModule::on_load(int argc, char8_t** argv)
{
    skr_log_set_level(SKR_LOG_LEVEL_INFO);
    SKR_LOG_INFO(u8"Scene Sample Mesh Module Loaded");

    skr::cmd::parser parser(argc, (char**)argv);
    parser.add(u8"gltf", u8"gltf file path", u8"-g", false);
    parser.add(u8"use-gltf", u8"whether to use gltf file", u8"-u", false);

    if (!parser.parse())
    {
        SKR_LOG_ERROR(u8"Failed to parse command line arguments.");
        return;
    }
    auto gltf_path_opt = parser.get_optional<skr::String>(u8"gltf");
    if (gltf_path_opt)
    {
        gltf_path = *gltf_path_opt;
        SKR_LOG_INFO(u8"gltf file path set to: %s", gltf_path.c_str());
    }
    else
    {
        SKR_LOG_INFO(u8"No gltf file specified");
    }

    auto use_gltf_opt = parser.get_optional<bool>(u8"use-gltf");
    if (use_gltf_opt)
    {
        use_gltf = *use_gltf_opt;
        SKR_LOG_INFO(u8"use gltf: %s", use_gltf ? u8"true" : u8"false");
    }
    else
    {
        use_gltf = true; // default to true
        SKR_LOG_INFO(u8"use gltf: %s", use_gltf ? u8"true" : u8"false");
    }

    scheduler.initialize({});
    scheduler.bind();

    world.initialize();
    render_device = SkrRendererModule::Get()->get_render_device();

    auto resourceRoot = (skr::fs::current_directory() / u8"../resources");
    skr_vfs_desc_t vfs_desc = {};
    vfs_desc.mount_type = SKR_MOUNT_TYPE_CONTENT;
    vfs_desc.override_mount_dir = resourceRoot.c_str();
    resource_vfs = skr_create_vfs(&vfs_desc);

    auto projectRoot = skr::fs::current_directory() / u8"../resources";
    skd::SProjectConfig projectConfig = {
        .assetDirectory = (projectRoot / u8"assets").string().c_str(),
        .resourceDirectory = (projectRoot / u8"resources").string().c_str(),
        .artifactsDirectory = (projectRoot / u8"artifacts").string().c_str()
    };
    skr::String projectName = u8"SceneSampleMesh";
    skr::String rootPath = projectRoot.string().c_str();
    project.OpenProject(u8"SceneSampleMesh", rootPath.c_str(), projectConfig);

    {
        InitializeReosurceSystem();
        InitializeAssetSystem();
    }
    scene_renderer = skr::SceneRenderer::Create();
    scene_renderer->initialize(render_device, &world, resource_vfs);
}

void SceneSampleMeshModule::on_unload()
{
    project.CloseProject();
    scene_renderer->finalize(SkrRendererModule::Get()->get_render_device());
    skr::SceneRenderer::Destroy(scene_renderer);
    // stop all ram_service
    {
        DestroyAssetSystem();
        DestroyResourceSystem();
    }
    world.finalize();
    scheduler.unbind();
    SKR_LOG_INFO(u8"Scene Sample Mesh Module Unloaded");
}

void SceneSampleMeshModule::CookAndLoadGLTF()
{
    auto& System = *skd::asset::GetCookSystem();

    // source file is a GLTF so we create a gltf importer, if it is a fbx, then just use a fbx importer
    auto importer = skd::asset::GltfMeshImporter::Create<skd::asset::GltfMeshImporter>();

    // static mesh has some additional meta data to append
    auto metadata = skd::asset::MeshAsset::Create<skd::asset::MeshAsset>();
    metadata->vertexType = u8"1b357a40-83ff-471c-8903-23e99d95b273"_guid; // GLTFVertexLayoutWithoutTangentId

    auto asset = skr::RC<skd::asset::AssetMetaFile>::New(
        u8"girl.gltf.meta",                             // virtual uri for this asset in the project
        MeshAssetID,                                    // guid for this asset
        skr::type_id_of<skr::renderer::MeshResource>(), // output resource is a mesh resource
        skr::type_id_of<skd::asset::MeshCooker>()       // this cooker cooks the raw mesh data to mesh resource
    );
    // source file
    // importer->assetPath = u8"D:/ws/repos/SakuraEngine/samples/application/game/assets/sketchfab/loli/scene.gltf";
    importer->assetPath = gltf_path.c_str();
    System.ImportAssetMeta(&project, asset, importer, metadata);
    auto event = System.EnsureCooked(asset->GetGUID());
    event.wait(true);
}

int SceneSampleMeshModule::main_module_exec(int argc, char8_t** argv)
{
    constexpr float kPi = rtm::constants::pi();
    SkrZoneScopedN("SceneSampleMeshModule::main_module_exec");
    SKR_LOG_INFO(u8"Running Scene Sample Mesh Module");

    SKR_LOG_INFO(u8"gltf file path: {%s}", gltf_path.c_str());
    if (use_gltf && gltf_path.is_empty())
    {
        SKR_LOG_ERROR(u8"gltf file path is empty, please specify a valid gltf file path.");
        return 1;
    }

    temp::Camera camera;
    scene_renderer->temp_set_camera(&camera);

    auto cgpu_device = render_device->get_cgpu_device();
    auto gfx_queue = render_device->get_gfx_queue();

    // transform mesh_buffer_t into CGPUBufferId
    skr_mesh_resource_t mesh_resource = {};
    skr_render_mesh_id render_mesh = mesh_resource.render_mesh = SkrNew<skr_render_mesh_t>();
    utils::Grid2DMesh dummy_mesh;
    skr::resource::AsyncResource<skr::renderer::MeshResource> gltf_mesh_resource;
    gltf_mesh_resource = MeshAssetID;

    if (use_gltf)
    {
        CookAndLoadGLTF();
    }
    else
    {

        dummy_mesh.init();
        dummy_mesh.generate_render_mesh(render_device, render_mesh);
    }

    {

        skr::render_graph::RenderGraphBuilder graph_builder;
        graph_builder.with_device(cgpu_device)
            .with_gfx_queue(gfx_queue)
            .enable_memory_aliasing();

        skr::SystemWindowCreateInfo main_window_info = {
            .title = skr::format(u8"Scene Viewer [{}]", gCGPUBackendNames[cgpu_device->adapter->instance->backend]),
            .size = { 1024, 768 },
        };

        imgui_app = skr::UPtr<skr::ImGuiApp>::New(main_window_info, render_device, graph_builder);
        imgui_app->initialize();
        imgui_app->enable_docking();
    }
    auto render_graph = imgui_app->render_graph();
    // Time
    SHiresTimer tick_timer;
    int64_t elapsed_us = 0;
    int64_t elapsed_frame = 0;
    int64_t fps = 60;
    skr_init_hires_timer(&tick_timer);
    uint64_t frame_index = 0;

    skr::input::Input::Initialize();

    auto resource_system = skr::resource::GetResourceSystem();

    while (!imgui_app->want_exit().comsume())
    {
        SkrZoneScopedN("LoopBody");
        {
            SkrZoneScopedN("PumpMessage");
            imgui_app->pump_message();
        }
        {
            // ResourceSystem Update
            resource_system->Update();
        }

        // Camera control
        {
            ImGuiIO& io = ImGui::GetIO();

            const float camera_speed = 0.0025f * io.DeltaTime;
            const float camera_pan_speed = 0.0025f * io.DeltaTime;
            const float camera_sensitivity = 0.05f;

            skr_float3_t world_up = { 0.0f, 1.0f, 0.0f };
            skr_float3_t camera_right = skr::normalize(skr::cross(camera.front, world_up));
            skr_float3_t camera_up = skr::normalize(skr::cross(camera_right, camera.front));

            // Movement
            if (ImGui::IsKeyDown(ImGuiKey_W)) camera.position += camera.front * camera_speed;
            if (ImGui::IsKeyDown(ImGuiKey_S)) camera.position -= camera.front * camera_speed;
            if (ImGui::IsKeyDown(ImGuiKey_A)) camera.position -= camera_right * camera_speed;
            if (ImGui::IsKeyDown(ImGuiKey_D)) camera.position += camera_right * camera_speed;

            // Rotation
            if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
            {
                static float yaw = 90.0f;
                static float pitch = 0.0f;
                yaw += io.MouseDelta.x * camera_sensitivity;
                pitch -= io.MouseDelta.y * camera_sensitivity;
                if (pitch > 89.0f) pitch = 89.0f;
                if (pitch < -89.0f) pitch = -89.0f;

                skr_float3_t direction;
                direction.x = cosf(yaw * (float)kPi / 180.f) * cosf(pitch * (float)kPi / 180.f);
                direction.y = sinf(pitch * (float)kPi / 180.f);
                direction.z = sinf(yaw * (float)kPi / 180.f) * cosf(pitch * (float)kPi / 180.f);
                camera.front = skr::normalize(direction);
            }

            // Panning
            if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
            {
                camera.position -= camera_right * io.MouseDelta.x * camera_pan_speed;
                camera.position += camera_up * io.MouseDelta.y * camera_pan_speed;
            }

            {
                ImGui::Begin("Camera Info");
                ImGui::Text("Position: (%.2f, %.2f, %.2f)", camera.position.x, camera.position.y, camera.position.z);
                ImGui::Text("Front:    (%.2f, %.2f, %.2f)", camera.front.x, camera.front.y, camera.front.z);
                ImGui::End();
            }
        }

        {

            // update viewport
            SkrZoneScopedN("Viewport Render");
            imgui_app->acquire_frames();
            auto main_window = imgui_app->main_window();
            const auto size = main_window->get_physical_size();
            camera.aspect = (float)size.x / (float)size.y;
            if (use_gltf)
            {
                gltf_mesh_resource.resolve(true, 0, ESkrRequesterType::SKR_REQUESTER_SYSTEM);
                if (gltf_mesh_resource.is_resolved())
                {
                    skr_mesh_resource_t* MeshResource = gltf_mesh_resource.get_resolved(true);
                    if (MeshResource)
                    {
                        if (MeshResource->render_mesh)
                        {
                            scene_renderer->draw_primitives(render_graph, MeshResource->render_mesh->primitive_commands);
                        }
                    }
                }
            }
            else
            {
                scene_renderer->draw_primitives(render_graph, render_mesh->primitive_commands);
            }
        };

        {
            SkrZoneScopedN("ImGuiRender");
            imgui_app->set_load_action(CGPU_LOAD_ACTION_LOAD);
            imgui_app->render_imgui();
        }
        {
            frame_index = render_graph->execute();
            if (frame_index >= RG_MAX_FRAME_IN_FLIGHT * 10)
                render_graph->collect_garbage(frame_index - RG_MAX_FRAME_IN_FLIGHT * 10);
        }
        // present
        imgui_app->present_all();
    }

    cgpu_wait_queue_idle(gfx_queue);
    imgui_app->shutdown();
    skr::input::Input::Finalize();
    skr_render_mesh_free(render_mesh);
    if (use_gltf)
    {
        mesh_resource.bins.clear();
        mesh_resource.render_mesh = nullptr;
    }
    else
    {
        dummy_mesh.destroy();
    }
    return 0;
}