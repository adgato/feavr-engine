#pragma once

#include <unordered_set>

#include "AutoCompleteWidget.h"
#include "assets-system/AssetManager.h"
#include "components/Tags.h"
#include "ecs/Engine.h"
#include "ecs/EngineView.h"
#include "glm/vec2.hpp"
#include "rendering/pass-system/Material.h"
#include "rendering/pass-system/PassComponent.h"
#include "rendering/pass-system/PassSystem.h"
#include "rendering/pass-system/StencilOutlinePass.h"

class Scene;
class IdentifyPass;

namespace ecs
{
    class EngineWidget
    {
        struct ShowTypeData
        {
            TypeID type;
            widgets::AutoCompleteWidget widget;
        };

        struct EngineViewData
        {
            std::vector<EntityID> showEntities {};
            std::vector<ShowTypeData> showTypes {};

            Tags includeTags;
            Tags includeComponents;
            Tags excludeTags;
            Tags excludeComponents;
        };

        widgets::AutoCompleteWidget includeTagsSelector {};
        widgets::AutoCompleteWidget excludeTagsSelector {};

        std::vector<std::string> typeNames {};
        std::unordered_set<std::string> typeNameSet {};
        std::vector<std::string> tags {};
        std::unordered_set<std::string> tagsSet {};

        std::vector<std::string> sceneAssetNames {};
        std::vector<assets_system::AssetID> sceneAssetIDs {};
        widgets::AutoCompleteWidget sceneAssetSelector {};

        char saveAssetName[256] {};
        int loadAssetIndex;

        std::array<EngineViewData, 10> engineViewTabs;
        size_t currentTab = 0;

        Scene& scene;
        Engine& engine;

        EngineView<Tags> tagView;
        EngineView<PassComponent<StencilOutlinePass>> outlineView;
        rendering::Material<StencilOutlinePass> outlineMat;

        glm::vec2 hotViewPos {};
        bool hotChanged = false;
        bool showHot = false;

        bool showMainMenu = false;

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

        std::vector<TypeID> GetComponentIDs(const Tags& components) const;

        void ShowUpdatePopup(Entity e, TypeID type);

        void RefeshTags();

        void RefreshAssets();

        void ShowTabs();

        void MainMenu();

        bool ConsiderEntity(EntityID e, const std::vector<TypeID>& includeTypes, const std::vector<TypeID>& excludeTypes);

        static bool IsEntityRelevant(const std::vector<TypeID>& types, const std::vector<TypeID>& includeTypes, const std::vector<TypeID>& excludeTypes);

        void Search();

        void SearchTree();

        void SetWindowSplit();

        void EngineTable();

    public:
        bool quitRequested = false;

        explicit EngineWidget(Scene& core);

        void SetHotEntity(Entity hotEntity, glm::vec2 coord);

        void Windows();
    };
}
