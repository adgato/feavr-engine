set(ENGINE_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/engine")

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "${ENGINE_SOURCE_DIR}/*.cpp"
    "${ENGINE_SOURCE_DIR}/*.hpp"
    "${ENGINE_SOURCE_DIR}/*.h"
)

add_executable(engine ${ENGINE_SOURCES})
target_include_directories(engine PUBLIC "${ENGINE_SOURCE_DIR}")
set_property(TARGET engine PROPERTY CXX_STANDARD 20)

target_compile_definitions(engine PUBLIC 
    GLM_FORCE_DEPTH_ZERO_TO_ONE 
    PROJECT_ROOT="${PROJECT_SOURCE_DIR}"
)

target_link_libraries(engine PUBLIC directmath vma glm Vulkan::Vulkan fmt::fmt stb_image vkbootstrap imgui fastgltf::fastgltf)
target_link_libraries(engine ${SDL2_LIBRARIES})
target_include_directories(engine PRIVATE ${SDL2_INCLUDE_DIRS})

target_precompile_headers(engine PUBLIC 
    <optional> 
    <vector> 
    <memory> 
    <string> 
    <unordered_map> 
    <glm/mat4x4.hpp>  
    <glm/vec4.hpp> 
    <vulkan/vulkan.h>
)
