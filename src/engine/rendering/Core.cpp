#include "Core.h"

#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
#include <thread>

#include "hlsl++/vector_int.h"
#include "imgui.h"
#include "assets-system/AssetManager.h"
#include "assets-system/lookup/Asset29.h"
#include "utility/ClickOnMeshTool.h"
#include "assets-system/lookup/Asset31.h"
#include "ecs/EngineExtensions.h"

void Core::Init()
{
    resources.Init();
    imguiOverlay.Init(resources.resource);
    renderer.Init(resources);
    LoadScene(assets_system::lookup::SCNE_structure);
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

    saveAsset.header["Meshes Start"] = static_cast<uint64_t>(renderer.passManager.serializeInfo.meshesStart);
    saveAsset.header["Meshes Size"] = static_cast<uint64_t>(renderer.passManager.serializeInfo.meshesSizeBytes);

    saveAsset.WriteToBlob(m);
    saveAsset.Save(filePath);
}

void Core::LoadScene(const assets_system::AssetID other)
{
    const assets_system::AssetFile loadAsset = assets_system::AssetManager::LoadAsset(other);
    assert(loadAsset.HasFormat("SCNE", 0));

    serial::Stream m = loadAsset.ReadFromBlob();

    renderer.passManager.Destroy();
    engine.Destroy();
    renderer.passManager.Init();
    renderer.passManager.Serialize(m);
    engine.Serialize(m);
    engine.Refresh();
    renderer.passManager.ReplaceInvalidAssetSources(other);
    renderer.passManager.FixupReferences(engine);
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

    ecs::EntityID focus = ecs::BadMaxIndex;
    if (clickOnMeshTool.DrawWaitingSample(3)) //why 3?
    {
        ecs::Entity entity = clickOnMeshTool.SampleCoordinate();

        std::vector<ecs::EntityID> submeshes {};
        renderer.passManager.GetPass<identify_pass::Pass>().IdentifySubMeshesOf(entity, submeshes);

        // this wil go somewhere else, but for now its all here

        ecs::EngineView<stencil_outline_pass::Component> view(engine);
        for (auto [submesh, _] : view)
            engine.Remove<stencil_outline_pass::Component>(submesh);

        for (ecs::Entity submesh : submeshes)
        {
            stencil_outline_pass::Component component {};
            component.transforms->push_back(entity);
            engine.Add<stencil_outline_pass::Component>(submesh, component);
        }

        engine.Refresh();

        fmt::println("Clicked {}", entity);
        focus = entity;
    }

    // imgui new frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();

    ImGui::NewFrame();
    // Create a window called "My First Tool", with a menu bar.
    if (ImGui::Begin("Text List Window"))
    {
        ImGui::Text("Text Lines Display");
        ImGui::Separator();
        ecs::Widget(engine, focus);
    }

    ImGui::End();
    ImGui::Render();

    if (!skipDrawing)
    {
        auto [cmd, frame] = resources.BeginFrame();

        renderer.Draw(resources.frameCount, cmd, frame);
        imguiOverlay.Draw(cmd, frame);

        if (requestDrawMeshIndices)
        {
            hlslpp::int2 mousePos;
            SDL_GetMouseState(&mousePos.x, &mousePos.y);
            clickOnMeshTool.DrawMeshIndices(cmd, renderer.sceneData.view, mousePos);
        }

        resources.EndFrame();
    } else
        std::this_thread::sleep_for(std::chrono::milliseconds { 100 });

    return loopAgain;
}

void Core::Destroy()
{
    renderer.Destroy();
    engine.Destroy();
    imguiOverlay.Destroy();
    clickOnMeshTool.Destroy();
    resources.Destroy();
}
