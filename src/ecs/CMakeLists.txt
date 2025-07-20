set(ECS_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/ecs")

file(GLOB_RECURSE ECS_HEADERS CONFIGURE_DEPENDS
        "${ECS_SOURCE_DIR}/*.hpp"
        "${ECS_SOURCE_DIR}/*.h"
)

file(GLOB_RECURSE ECS_SOURCES CONFIGURE_DEPENDS
        "${ECS_SOURCE_DIR}/*.cpp"
)

add_library(ecs STATIC ${ECS_SOURCES} ${ECS_HEADERS})
target_include_directories(ecs PUBLIC "${ECS_SOURCE_DIR}")
target_compile_features(ecs PUBLIC cxx_std_20)

target_link_libraries(ecs PUBLIC asset_system glm)