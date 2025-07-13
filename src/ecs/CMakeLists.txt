set(ECS_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/ecs")

file(GLOB_RECURSE ECS_HEADERS CONFIGURE_DEPENDS
        "${ECS_SOURCE_DIR}/*.hpp"
        "${ECS_SOURCE_DIR}/*.h"
)

# Use INTERFACE for header-only libraries
add_library(ecs INTERFACE)
target_include_directories(ecs INTERFACE "${ECS_SOURCE_DIR}")
target_compile_features(ecs INTERFACE cxx_std_20)

# Optionally add headers to target for IDE
target_sources(ecs INTERFACE ${ECS_HEADERS})