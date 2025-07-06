set(DRAFT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/shader_drafter")

file(GLOB_RECURSE DRAFT_SOURCES CONFIGURE_DEPENDS
    "${DRAFT_SOURCE_DIR}/*.cpp"
    "${DRAFT_SOURCE_DIR}/*.hpp"
    "${DRAFT_SOURCE_DIR}/*.h"
    "${DRAFT_SOURCE_DIR}/*.c"
)

add_executable(shader_drafter ${DRAFT_SOURCES})
target_include_directories(shader_drafter PUBLIC "${DRAFT_SOURCE_DIR}")
set_property(TARGET shader_drafter PROPERTY CXX_STANDARD 20)

target_compile_definitions(shader_drafter PUBLIC 
    PROJECT_ROOT="${PROJECT_SOURCE_DIR}"
)

target_link_libraries(shader_drafter PUBLIC Vulkan::Vulkan fmt::fmt)
