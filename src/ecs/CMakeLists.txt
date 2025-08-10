set(LIBRARY_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/ecs")

file(GLOB_RECURSE LIBRARY_HEADERS CONFIGURE_DEPENDS
        "${LIBRARY_SOURCE_DIR}/*.hpp"
        "${LIBRARY_SOURCE_DIR}/*.h"
)

file(GLOB_RECURSE LIBRARY_SOURCES CONFIGURE_DEPENDS
        "${LIBRARY_SOURCE_DIR}/*.cpp"
        "${LIBRARY_SOURCE_DIR}/*.c"
)


add_library(ecs STATIC ${LIBRARY_SOURCES} ${LIBRARY_HEADERS})
target_include_directories(ecs PUBLIC "${LIBRARY_SOURCE_DIR}")
target_link_libraries(ecs PUBLIC asset_system imgui)

target_compile_definitions(ecs PUBLIC
        PROJECT_ROOT="${PROJECT_SOURCE_DIR}"
)

target_precompile_headers(ecs PUBLIC
        <string>
        <memory>
        <array>
        <vector>
        <imgui.h>
)
