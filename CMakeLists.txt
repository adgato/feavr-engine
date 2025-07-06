cmake_minimum_required (VERSION 3.8)
cmake_policy(SET CMP0048 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)

project ("template")

find_package(Vulkan REQUIRED)

add_subdirectory(include)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

add_subdirectory(src/engine)
add_subdirectory(src/directmath)
add_subdirectory(src/shader_drafter)
add_subdirectory(shaders)

# organise

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT engine)

foreach(LIB engine directmath shaders shader_drafter)
    if(TARGET ${LIB})
        set_property(TARGET ${LIB} PROPERTY FOLDER "Core")
    endif()
endforeach()

foreach(LIB vma glm fmt stb_image vkbootstrap imgui fastgltf fastgltf_simdjson sdl_headers_copy shader_binaries)
    if(TARGET ${LIB})
        set_property(TARGET ${LIB} PROPERTY FOLDER "Include")
    endif()
endforeach()
