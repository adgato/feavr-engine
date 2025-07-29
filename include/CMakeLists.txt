# Use system SDL2 instead of bundled version
find_package(SDL2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LZ4 REQUIRED liblz4)

# Create an imported library target
add_library(lz4 INTERFACE IMPORTED)

# Set the properties on the imported target
set_target_properties(lz4 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LZ4_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${LZ4_LIBRARIES}"
        INTERFACE_COMPILE_OPTIONS "${LZ4_CFLAGS_OTHER}"
)

# If there are library directories, add them
if (LZ4_LIBRARY_DIRS)
    set_target_properties(lz4 PROPERTIES
            INTERFACE_LINK_DIRECTORIES "${LZ4_LIBRARY_DIRS}"
    )
endif ()

add_library(vkbootstrap STATIC)
add_library(imgui STATIC)
add_library(spirv_reflect STATIC)
add_library(glm INTERFACE)
add_library(vma INTERFACE)
add_library(hlslpp INTERFACE)
add_library(stb_image INTERFACE)

add_subdirectory(fastgltf)
add_subdirectory(fmt EXCLUDE_FROM_ALL)

# Remove this line - we're using system SDL2 now
# add_subdirectory(SDL EXCLUDE_FROM_ALL)

target_sources(vkbootstrap PRIVATE
        vkbootstrap/VkBootstrap.h
        vkbootstrap/VkBootstrap.cpp
)

target_sources(spirv_reflect PRIVATE
        spirv_reflect/spirv_reflect.h
        spirv_reflect/spirv_reflect.c
        spirv_reflect/include/spirv/unified1/spirv.h
)

target_include_directories(spirv_reflect PUBLIC spirv_reflect)
target_include_directories(vkbootstrap PUBLIC vkbootstrap)
target_link_libraries(vkbootstrap PUBLIC Vulkan::Vulkan $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>)
set_property(TARGET vkbootstrap PROPERTY CXX_STANDARD 20)

#both vma and glm and header only libs so we only need the include path
target_include_directories(vma INTERFACE vma)
target_include_directories(glm INTERFACE glm)
target_include_directories(hlslpp INTERFACE hlsl++)

target_include_directories(imgui PUBLIC imgui)

target_sources(imgui PRIVATE
        imgui/imgui.h
        imgui/imgui.cpp

        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_impl_vulkan.cpp
        imgui/imgui_impl_sdl2.cpp
)

# Use system SDL2 instead of bundled
target_link_libraries(imgui PUBLIC Vulkan::Vulkan ${SDL2_LIBRARIES})
target_include_directories(imgui PUBLIC ${SDL2_INCLUDE_DIRS})
