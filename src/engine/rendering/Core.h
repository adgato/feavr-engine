#pragma once
#include "ImguiOverlay.h"
#include "utility/ClickOnMeshTool.h"
#include "RenderingEngine.h"

class Core
{
public:
    ecs::Engine engine {};
    rendering::RenderingResources resources {};
    RenderingEngine renderer { engine, resources };
    ImguiOverlay imguiOverlay {};
    rendering::ClickOnMeshTool clickOnMeshTool { resources, renderer.passManager };

    bool skipDrawing = false;
    
    void Init();

    void SaveScene(const char* filePath);

    void LoadScene(assets_system::AssetID other);

    bool Next();
    void Destroy();
};
