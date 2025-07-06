set(MATH_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/directmath")

file(GLOB_RECURSE MATH_SOURCES CONFIGURE_DEPENDS
    "${MATH_SOURCE_DIR}/*.cpp"
    "${MATH_SOURCE_DIR}/*.hpp"
    "${MATH_SOURCE_DIR}/*.h"
)

add_library(directmath STATIC ${MATH_SOURCES})
target_include_directories(directmath PUBLIC "${MATH_SOURCE_DIR}")

target_compile_definitions(directmath PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)

target_link_libraries(directmath PUBLIC glm)

set_property(TARGET directmath PROPERTY CXX_STANDARD 20)