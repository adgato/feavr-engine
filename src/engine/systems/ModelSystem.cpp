#include "ModelSystem.h"
#include "rendering/pass-system/Mesh.h"

namespace systems
{
    void ModelSystem::Update(const Camera& camera)
    {
        for (auto [id, model, transform] : view)
        {
            if (transform.hasChanged)
            {
                model.Update(engine, transform);
                transform.hasChanged = false;
            }
            model.RefreshCull(camera);
        }
    }
}
