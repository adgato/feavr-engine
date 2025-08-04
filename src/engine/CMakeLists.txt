set(LIBRARY_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/engine")

file(GLOB_RECURSE LIBRARY_HEADERS CONFIGURE_DEPENDS
        "${LIBRARY_SOURCE_DIR}/*.hpp"
        "${LIBRARY_SOURCE_DIR}/*.h"
)

file(GLOB_RECURSE LIBRARY_SOURCES CONFIGURE_DEPENDS
        "${LIBRARY_SOURCE_DIR}/*.cpp"
        "${LIBRARY_SOURCE_DIR}/*.c"
)

add_executable(engine rendering/main.cpp)

target_sources(engine PUBLIC ${LIBRARY_SOURCES} ${LIBRARY_HEADERS})

target_include_directories(engine PUBLIC "${LIBRARY_SOURCE_DIR}")

target_compile_definitions(engine PUBLIC
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        PROJECT_ROOT="${PROJECT_SOURCE_DIR}"
)

target_link_libraries(engine PUBLIC ecs asset_system hlslpp vma glm Vulkan::Vulkan fmt::fmt stb_image vkbootstrap imgui fastgltf::fastgltf stdc++exp)
target_link_libraries(engine ${SDL2_LIBRARIES})
target_include_directories(engine PRIVATE ${SDL2_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-nullability-completeness -g3 -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls")

target_precompile_headers(engine PUBLIC
        # Vulkan - huge header, used everywhere in graphics code
        <vulkan/vulkan.h>

        # SDL2 - stable, commonly included
        <SDL2/SDL.h>

        # ImGui - if you use it across many files
        <imgui.h>
        <imgui_impl_vulkan.h>
        <imgui_impl_sdl2.h>

        # Standard library headers you use a lot
        <string>
        <memory>
        <unordered_map>
        <array>
        <vector>
)
