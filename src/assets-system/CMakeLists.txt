set(LIBRARY_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/assets-system")

file(GLOB_RECURSE LIBRARY_HEADERS CONFIGURE_DEPENDS
        "${LIBRARY_SOURCE_DIR}/*.hpp"
        "${LIBRARY_SOURCE_DIR}/*.h"
)

file(GLOB_RECURSE LIBRARY_SOURCES CONFIGURE_DEPENDS
        "${LIBRARY_SOURCE_DIR}/*.cpp"
        "${LIBRARY_SOURCE_DIR}/*.c"
)


add_library(asset_system STATIC ${LIBRARY_SOURCES} ${LIBRARY_HEADERS})
target_include_directories(asset_system PUBLIC "${LIBRARY_SOURCE_DIR}")
target_link_libraries(asset_system PUBLIC fmt::fmt lz4 spirv_reflect)

target_compile_definitions(asset_system PUBLIC
        PROJECT_ROOT="${PROJECT_SOURCE_DIR}"
)

target_precompile_headers(asset_system PUBLIC
        <vector>
        <string>
        <memory>
        <array>
)
