#include "Core.h"

#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
#include <thread>

#include "imgui.h"
#include "rendering/utility/OLD_loader.h"

void Core::Init()
{
    swapchain.Init();
    imguiOverlay.Init(swapchain.resource);
    engine.Init(&swapchain);
    loadGltf(this, {PROJECT_ROOT"/assets/structure.glb"});
}

bool Core::Next()
{
    SDL_Event e;
    bool loopAgain = true;

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

        engine.mainCamera.processSDLEvent(e);
        ImGui_ImplSDL2_ProcessEvent(&e);
    }

    // imgui new frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();

    ImGui::NewFrame();
    // Create a window called "My First Tool", with a menu bar.
    bool active = true;
    ImGui::ShowDemoWindow();
    ImGui::Begin("My First Tool", &active, ImGuiWindowFlags_MenuBar);
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
            if (ImGui::MenuItem("Save", "Ctrl+S"))   { /* Do stuff */ }
            if (ImGui::MenuItem("Close", "Ctrl+W"))  { /* Do stuff */ }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    static float col[4];

    // Edit a color stored as 4 floats
    ImGui::ColorEdit4("Color", col);

    // Generate samples and plot them
    float samples[100];
    for (int n = 0; n < 100; n++)
        samples[n] = sinf(n * 0.2f + ImGui::GetTime() * 1.5f);
    ImGui::PlotLines("Samples", samples, 100);

    // Display contents in a scrolling region
    ImGui::TextColored(ImVec4(1,1,0,1), "Important Stuff");
    ImGui::BeginChild("Scrolling");
    for (int n = 0; n < 50; n++)
        ImGui::Text("%04d: Some text", n);
    ImGui::EndChild();
    ImGui::End();
    ImGui::Render();

    if (!skipDrawing)
    {
        auto [cmd, frame] = swapchain.BeginFrame();
        engine.Draw(swapchain.frameCount, cmd, frame);
        imguiOverlay.Draw(cmd, frame);
        swapchain.EndFrame();
    } else
        std::this_thread::sleep_for(std::chrono::milliseconds {100});

    return loopAgain;
}

void Core::Destroy()
{
    engine.Destroy();
    imguiOverlay.Destroy();
    swapchain.Destroy();
}
