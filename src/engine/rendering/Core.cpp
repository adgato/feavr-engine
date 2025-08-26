#include "Core.h"

#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
#include <thread>

#include "imgui.h"
#include "assets-system/AssetManager.h"
#include "assets-system/lookup/Asset29.h"
#include "assets-system/lookup/Asset31.h"
#include "pass-system/Mesh.h"
#include "utility/ClickOnMeshTool.h"
#include "widgets/EngineWidget.h"

void Core::Init()
{
    resources.Init();
    imguiOverlay.Init(resources.resource);
    renderer.Init(resources);
    LoadScene(assets_system::lookup::SCNE_structure);
    LoadScene(assets_system::lookup::SCNE_falcon2);
    clickOnMeshTool.Init();
}

void Core::SaveScene(const char* filePath)
{
    serial::Stream m;
    m.InitWrite();
    renderer.passManager.Serialize(m);

    engine.Refresh();
    engine.Serialize(m);

    assets_system::AssetFile saveAsset("SCNE", 0);
    saveAsset.WriteToBlob(m);
    saveAsset.Save(filePath);
}

void Core::LoadScene(const assets_system::AssetID other)
{
    const assets_system::AssetFile loadAsset = assets_system::AssetManager::LoadAsset(other);
    assert(loadAsset.HasFormat("SCNE", 0));

    serial::Stream m = loadAsset.ReadFromBlob();

    renderer.passManager.Serialize(m);
    engine.Serialize(m);
    engine.Refresh();

    ecs::EngineView<Mesh> submeshView(engine);
    for (auto [id, mesh] : submeshView)
    {
        if (!mesh.IsValid())
            mesh.UploadMesh(resources);
    }
}

bool Core::Next()
{
    SDL_Event e;
    bool loopAgain = true;

    bool requestDrawMeshIndices = false;

    //Handle events on queue
    while (SDL_PollEvent(&e) != 0)
    {
        switch (e.type)
        {
            case SDL_QUIT:
                loopAgain = false;
                break;
            case SDL_WINDOWEVENT_MINIMIZED:
                skipDrawing = true;
                break;
            case SDL_WINDOWEVENT_MAXIMIZED:
                skipDrawing = false;
                break;
            default: // SDL_WINDOWEVENT_RESIZED
                break;
        }

        ImGui_ImplSDL2_ProcessEvent(&e);
        if (!ImGui::GetIO().WantCaptureMouse)
        {
            renderer.mainCamera.processSDLEvent(e);
            requestDrawMeshIndices |= e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT;
        }
    }

    if (clickOnMeshTool.DrawWaitingSample(3)) //why 3?
    {
        // this wil go somewhere else, but for now its all here

        ecs::EngineView<PassComponent<StencilOutlinePass>> view(engine);
        for (auto [submesh, _] : view)
            engine.Remove<PassComponent<StencilOutlinePass>>(submesh);

        ecs::Entity entity = clickOnMeshTool.SampleCoordinate();
        if (engine.IsValid(entity))
        {
            const rendering::Material<StencilOutlinePass> outlineMat { engine, renderer.passManager };
            outlineMat.Apply(entity, true);
        }

        engineWidget.SetHotEntity(entity, coord);
    }

    transformSys.Update(renderer.mainCamera);

    // imgui new frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    engineWidget.Windows();
    ImGui::Render();

    engine.Refresh();

    if (!skipDrawing)
    {
        auto [cmd, frame] = resources.BeginFrame();

        if (cmd)
        {
            renderer.Draw(resources.frameCount, cmd, frame);
            imguiOverlay.Draw(cmd, frame);

            if (requestDrawMeshIndices)
            {
                int w, h;
                SDL_GetMouseState(&w, &h);
                coord = glm::vec2 { w, h };
                clickOnMeshTool.DrawMeshIndices(cmd, frame.imageExtent, renderer.mainCamera, coord);
            }

            resources.EndFrame();
        }
    } else
        std::this_thread::sleep_for(std::chrono::milliseconds { 100 });

    return loopAgain;
}

void Core::Destroy()
{
    vkDeviceWaitIdle(resources.resource);
    engine.Destroy();
    renderer.Destroy();
    imguiOverlay.Destroy();
    clickOnMeshTool.Destroy();
    resources.Destroy();
}
