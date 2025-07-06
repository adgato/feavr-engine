#include "PassManager.h"

namespace rendering
{
    void PassManager::Init(VulkanEngine* e, ecs::EntityManager* m, const std::vector<std::tuple<TypeID, uint32_t>>& renderOrder)
    {
        engine = e;
        manager = m;
        
        order.reserve(renderOrder.size());
        drawMasks.reserve(renderOrder.size());
        for (const auto& [passType, mask] : renderOrder)
        {
            order.push_back(passType);
            drawMasks.push_back(mask);
        }
        
        passGroups.clear();
        passGroups.resize(renderOrder.size());
    }

    void PassManager::Draw(const VkCommandBuffer cmd, const uint32_t drawMask) const
    {
        for (uint32_t k = 0; k < passGroups.size(); ++k)
        {
            const std::shared_ptr<IPassGroup>& group = passGroups[k];

            if (!group || (drawMasks[k] & drawMask) == 0)
                continue;

            passes::Pass* pass = group->GetPass();
            
            pass->ConfigurePass(cmd);
            
            const uint32_t size = group->Count();
            for (uint32_t i = 0; i < size; ++i)
            {
                passes::PassInstance* instance = group->GetInstance(i);
                instance->ConfigureInstance(cmd);
                for (const uint32_t j : instance->meshIdxs)
                    pass->DrawMesh(cmd, meshes[j]);
            }
        }
    }

    void PassManager::Destroy()
    {
        for (const std::shared_ptr<IPassGroup>& group : passGroups)
        {
            if (!group)
                continue;
            const uint32_t size = group->Count();
            for (uint32_t i = 0; i < size; ++i)
                group->GetInstance(i)->Destroy();
            group->GetPass()->Destroy();
        }
        passGroups.clear();
        meshes.clear();
    }
}
