#pragma once
#include "SubMesh.h"
#include "ecs/Engine.h"
#include "passes/PassInstance.h"

namespace rendering
{
    class IPassGroup
    {
    public:
        IPassGroup() = default;
        virtual ~IPassGroup() = default;
        IPassGroup(const IPassGroup& other) = default;
        IPassGroup(IPassGroup&& other) = default;
        IPassGroup& operator=(const IPassGroup& other) = default;
        IPassGroup& operator=(IPassGroup&& other) = default;

        virtual passes::Pass* GetPass() = 0;
        virtual passes::PassInstance* GetInstance(uint32_t i) = 0;
        virtual uint32_t Count() = 0;
    };

    template <typename T> requires std::derived_from<T, passes::Pass>
    class PassGroup final : public IPassGroup
    {
    public:
        std::shared_ptr<T> pass;
        std::vector<passes::PassInst<T>> instances;

        passes::Pass* GetPass() override
        {
            return pass.get();
        }

        passes::PassInstance* GetInstance(uint32_t i) override
        {
            return &instances[i];
        }

        uint32_t Count() override
        {
            return static_cast<uint32_t>(instances.size());
        }
    };

    using OldTypeID = const int*;

    template <typename>
    struct TypeIdentifier
    {
        constexpr static int _id {};

        static constexpr OldTypeID id()
        {
            return &_id;
        }
    };

    template <typename T>
    constexpr OldTypeID GetTypeID()
    {
        return TypeIdentifier<T>::id();
    }

    class PassManager
    {
        VulkanEngine* engine = nullptr;
        ecs::EntityManager* manager = nullptr;

        std::vector<OldTypeID> order{};
        std::vector<uint32_t> drawMasks{};

    public:
        std::vector<std::shared_ptr<IPassGroup>> passGroups;
        std::vector<SubMesh> meshes;

        void Init(VulkanEngine* e, ecs::EntityManager* m, const std::vector<std::tuple<OldTypeID, uint32_t>>& renderOrder);

        template <typename T> requires std::derived_from<T, passes::Pass>
        uint32_t GetPassCount()
        {
            PassGroup<T>& passGroup = GetPassGroup<T>();
            return static_cast<uint32_t>(passGroup.instances.size());
        }
        
        template <typename T> requires std::derived_from<T, passes::Pass>
        passes::PassInst<T>& GetPassInstance(uint32_t index)
        {
            PassGroup<T>& passGroup = GetPassGroup<T>();
            assert(index < passGroup.instances.size());
            return passGroup.instances[index];
        }

        template <typename T> requires std::derived_from<T, passes::Pass>
        uint32_t AddPassInstance()
        {
            PassGroup<T>& passGroup = GetPassGroup<T>();
            const uint32_t index = static_cast<uint32_t>(passGroup.instances.size());
            passGroup.instances.emplace_back().Init(passGroup.pass);
            return index;
        }

        void Draw(VkCommandBuffer cmd, uint32_t drawMask) const;

        void Destroy();

    private:
        template <typename T> requires std::derived_from<T, passes::Pass>
        PassGroup<T>& GetPassGroup()
        {
            uint32_t i;
            for (i = 0; i < order.size(); ++i)
                if (GetTypeID<T>() == order[i])
                    break;

            assert(i < order.size());
            
            std::shared_ptr<PassGroup<T>> passGroup;

            if (passGroups[i])
            {
                passGroup = std::static_pointer_cast<PassGroup<T>>(passGroups[i]);
            }
            else
            {
                passGroup = std::make_shared<PassGroup<T>>();
                passGroup->pass = std::make_shared<T>();
                passGroup->pass->Init(engine, manager);
                passGroups[i] = std::static_pointer_cast<IPassGroup>(passGroup);
            }

            return *passGroup;
        }
    };
}
