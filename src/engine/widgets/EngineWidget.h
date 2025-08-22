#pragma once

#include <unordered_set>

#include "AutoCompleteWidget.h"
#include "components/Tags.h"
#include "ecs/Engine.h"
#include "ecs/EngineView.h"
#include "glm/vec2.hpp"

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
        glm::vec2 hotViewPos {};
        bool hotChanged = false;
        bool showHot = false;
        float split_ratio = 0.7f;

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


        void EngineTable();

    public:
        explicit EngineWidget(Engine& engine);

        void SetHotEntity(Entity hotEntity, glm::vec2 coord);

        void Windows();
    };
}
