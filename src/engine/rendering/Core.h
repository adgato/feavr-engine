#pragma once
#include "ImguiOverlay.h"
#include "utility/ClickOnMeshTool.h"
#include "RenderingEngine.h"
#include "assets-system/AssetID.h"
#include "systems/ModelSystem.h"
#include "widgets/EngineWidget.h"

class Core
{
public:
    ecs::Engine engine {};
    rendering::RenderingResources resources {};
    RenderingEngine renderer { engine, resources };
    ImguiOverlay imguiOverlay {};
    rendering::ClickOnMeshTool clickOnMeshTool { resources, renderer.passManager };
    ecs::EngineWidget engineWidget { engine, renderer.passManager };
    systems::ModelSystem transformSys { engine };

    bool skipDrawing = false;
    glm::vec2 coord;

    void Init();

    void SaveScene(const char* filePath);

    void LoadScene(assets_system::AssetID other);

    bool Next();

    void Destroy();
};
