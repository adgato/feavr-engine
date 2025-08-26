#pragma once

#include <unordered_set>

#include "AutoCompleteWidget.h"
#include "components/Tags.h"
#include "ecs/Engine.h"
#include "ecs/EngineView.h"
#include "glm/vec2.hpp"
#include "rendering/pass-system/Material.h"
#include "rendering/pass-system/PassComponent.h"
#include "rendering/pass-system/PassSystem.h"
#include "rendering/pass-system/StencilOutlinePass.h"

class IdentifyPass;

namespace ecs
{
    class EngineWidget
    {
        std::vector<std::string> typeNames {};
        std::unordered_set<std::string> typeNameSet {};
        std::vector<std::string> tags {};
        std::unordered_set<std::string> tagsSet {};

        struct ShowTypeData
        {
            TypeID type;
            widgets::AutoCompleteWidget widget;
        };

        std::vector<EntityID> showEntities {};
        std::vector<ShowTypeData> showTypes {};
        widgets::AutoCompleteWidget includeTagsSelector {};
        widgets::AutoCompleteWidget excludeTagsSelector {};

        Tags includeTags;
        Tags includeComponents;
        Tags excludeTags;
        Tags excludeComponents;
        Engine& engine;
        EngineView<Tags> tagView;
        EngineView<PassComponent<StencilOutlinePass>> outlineView;
        rendering::Material<StencilOutlinePass> outlineMat;
        glm::vec2 hotViewPos {};
        bool hotChanged = false;
        bool showHot = false;

        struct UpdateSplitData
        {
            float ratio = 0.3f;
            float target = 0.3f;
            float prev = 0.3f;
            bool toggleOn = true;
            bool dragging = false;
        } split {};

        struct UpdatePopupData
        {
            enum { DISABED, ADD, REMOVE } mode;
            EntityID entity;
            TypeID type;
        } popup {};

        std::vector<TypeID> GetComponentIDs(const Tags& components);

        void ShowUpdatePopup(Entity e, TypeID type);

        void RefeshTags();

        bool ConsiderEntity(EntityID e, const std::vector<TypeID>& includeTypes, const std::vector<TypeID>& excludeTypes);

        static bool IsEntityRelevant(const std::vector<TypeID>& types, const std::vector<TypeID>& includeTypes, const std::vector<TypeID>& excludeTypes);

        void Search();

        void SearchTree();

        void SetWindowSplit();


        void EngineTable();

    public:
        explicit EngineWidget(Engine& engine, rendering::PassSystem& passSys);

        void SetHotEntity(Entity hotEntity, glm::vec2 coord);

        void Windows();
    };
}
