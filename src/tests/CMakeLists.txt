set(LIBRARY_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/tests")

file(GLOB_RECURSE LIBRARY_HEADERS CONFIGURE_DEPENDS
        "${LIBRARY_SOURCE_DIR}/*.h"
)

file(GLOB_RECURSE LIBRARY_SOURCES CONFIGURE_DEPENDS
        "${LIBRARY_SOURCE_DIR}/*.cpp"
)

add_executable(tests tests/main.cpp)

target_sources(tests PUBLIC ${LIBRARY_SOURCES} ${LIBRARY_HEADERS})


target_include_directories(tests PUBLIC "${LIBRARY_SOURCE_DIR}")

target_compile_definitions(tests PUBLIC
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        PROJECT_ROOT="${PROJECT_SOURCE_DIR}"
)

target_link_libraries(tests PUBLIC ecs asset_system)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-nullability-completeness") # -g3 -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls

target_precompile_headers(tests PUBLIC
        <string>
        <memory>
        <unordered_map>
        <array>
        <vector>
)
