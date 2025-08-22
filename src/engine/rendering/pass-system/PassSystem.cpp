#include "PassSystem.h"
#include "Mesh.h"
#include "serialisation/array.h"
#include <map>

#include "assets-system/AssetManager.h"
#include "ecs/EngineView.h"
#include "rendering/RenderingEngine.h"
#include "rendering/resources/RenderingResources.h"

namespace rendering
{
    void PassSystem::Init()
    {
        defaultPass.Init();
        identifyPass.Init();
        stencilOutlinePass.Init();
    }

    void PassSystem::Draw(const VkCommandBuffer cmd)
    {
        defaultPass.Draw(cmd);
        stencilOutlinePass.Draw(cmd);
    }

    void PassSystem::Destroy()
    {
        defaultPass.Destroy();
        identifyPass.Destroy();
        stencilOutlinePass.Destroy();
    }
}
