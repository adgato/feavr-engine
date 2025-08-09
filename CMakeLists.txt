cmake_minimum_required (VERSION 3.10)
cmake_policy(SET CMP0048 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)

set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_UNITY_BUILD ON)

project ("feavr-engine")

find_package(Vulkan REQUIRED)

add_subdirectory(include)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

add_subdirectory(src/ecs)
add_subdirectory(src/assets-system)
add_subdirectory(src/engine)
add_subdirectory(src/tests)