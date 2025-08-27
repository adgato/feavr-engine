#pragma once
#include "ImguiOverlay.h"
#include "utility/ClickOnMeshTool.h"
#include "RenderingEngine.h"
#include "systems/ModelSystem.h"
#include "widgets/EngineWidget.h"

class Scene
{
public:
    ecs::Engine engine {};
    rendering::RenderingResources resources {};
    RenderingEngine renderer { engine, resources };
    ImguiOverlay imguiOverlay {};
    rendering::ClickOnMeshTool clickOnMeshTool { resources, renderer.passSys };
    systems::ModelSystem transformSys { engine };
    ecs::EngineWidget engineWidget { *this };

    bool skipDrawing = false;
    glm::vec2 coord;

    void Init();

    bool Next();

    void Save(const char* path);
    void Load(assets_system::AssetID assetID);

    void Destroy();
};
