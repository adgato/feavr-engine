#include "Core.h"

#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
#include <thread>

#include "hlsl++/vector_int.h"
#include "imgui.h"
#include "utility/ClickOnMeshTool.h"
#include "assets-system/lookup/Asset29.h"
#include "utility/Screenshot.h"

void Core::Init()
{
    swapchain.Init();
    imguiOverlay.Init(swapchain.resource);
    engine.Init(&swapchain);
    engine.LoadScene(assets_system::lookup::SCNE_structure);
}

bool Core::Next()
{
    SDL_Event e;
    bool loopAgain = true;

    bool clickedScene = false;

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
            clickedScene |= e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT;
        }
    }

    if (clickOnMeshTool.SelectMeshCompleted())
    {
        hlslpp::int2 mousePos;
        SDL_GetMouseState(&mousePos.x, &mousePos.y);
        ecs::Entity entity = clickOnMeshTool.SampleCoordinate(mousePos);
        clickOnMeshTool.Destroy();
        fmt::println("Clicked {}", entity);
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
        engine.ecsEngine.Wiget();
    }

    ImGui::End();
    ImGui::Render();

    if (!skipDrawing)
    {
        auto [cmd, frame] = swapchain.BeginFrame();

        engine.Draw(swapchain.frameCount, cmd, frame);
        imguiOverlay.Draw(cmd, frame);

        if (clickedScene && clickOnMeshTool.Destroyed())
        {
            clickOnMeshTool.Init(engine.drawImage, engine.depthImage);
            clickOnMeshTool.SelectMesh(cmd);
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
