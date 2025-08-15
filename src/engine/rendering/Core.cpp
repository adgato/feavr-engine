#include "Core.h"

#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
#include <thread>

#include "hlsl++/vector_int.h"
#include "imgui.h"
#include "assets-system/lookup/Asset29.h"
#include "utility/ClickOnMeshTool.h"
#include "assets-system/lookup/Asset31.h"
#include "utility/Screenshot.h"
#include "ecs/EngineExtensions.h"

void Core::Init()
{
    swapchain.Init();
    imguiOverlay.Init(swapchain.resource);
    engine.Init(&swapchain);
    engine.LoadScene(assets_system::lookup::SCNE_falcon2);
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
            engine.mainCamera.processSDLEvent(e);
            requestDrawMeshIndices |= e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT && clickOnMeshTool.Destroyed();
        }
    }

    ecs::EntityID focus = ecs::BadMaxIndex;
    if (clickOnMeshTool.DrawCompleted())
    {
        hlslpp::int2 mousePos;
        SDL_GetMouseState(&mousePos.x, &mousePos.y);
        ecs::Entity entity = clickOnMeshTool.SampleCoordinate(mousePos);
        clickOnMeshTool.Destroy();

        std::vector<ecs::EntityID> submeshes {};
        engine.passManager.GetPass<unlit_pass::Pass>().IdentifySubMeshesOf(entity, submeshes);

        // this wil go somewhere else, but for now its all here

        ecs::EngineView<stencil_outline_pass::Component> view(engine.ecsEngine);
        for (auto [submesh, _] : view)
            engine.ecsEngine.Remove<stencil_outline_pass::Component>(submesh);

        for (ecs::Entity submesh : submeshes)
        {
            stencil_outline_pass::Component component {};
            component.transforms->push_back(entity);
            engine.ecsEngine.Add<stencil_outline_pass::Component>(submesh, component);
        }

        engine.ecsEngine.Refresh();

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
        ecs::Widget(engine.ecsEngine, focus);
    }

    ImGui::End();
    ImGui::Render();

    if (!skipDrawing)
    {
        auto [cmd, frame] = swapchain.BeginFrame();

        engine.Draw(swapchain.frameCount, cmd, frame);
        imguiOverlay.Draw(cmd, frame);

        if (requestDrawMeshIndices)
        {
            clickOnMeshTool.Init(engine.drawImage.imageExtent);
            clickOnMeshTool.DrawMeshIndices(cmd);
        }

        swapchain.EndFrame();
    } else
        std::this_thread::sleep_for(std::chrono::milliseconds { 100 });

    return loopAgain;
}

void Core::Destroy()
{
    engine.Destroy();
    imguiOverlay.Destroy();
    clickOnMeshTool.Destroy();
    swapchain.Destroy();
}
