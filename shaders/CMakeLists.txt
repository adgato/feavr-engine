set(SHADER_SOURCE_DIR "${PROJECT_SOURCE_DIR}/shaders")
set(SHADER_BINARY_DIR "${PROJECT_SOURCE_DIR}/shaders/spv")

file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)
find_program(DXC_COMPILER dxc HINTS /usr/bin /usr/local/bin /home/adgato/portable/dxc/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

file(GLOB_RECURSE GLSL_SOURCE_FILES CONFIGURE_DEPENDS
    "${SHADER_SOURCE_DIR}/*.frag"
    "${SHADER_SOURCE_DIR}/*.vert"
    "${SHADER_SOURCE_DIR}/*.comp"
)
file(GLOB_RECURSE HLSL_SOURCE_FILES CONFIGURE_DEPENDS
    "${SHADER_SOURCE_DIR}/*.hlsl"
)

function(get_hlsl_entry_points HLSL_FILE OUT_ENTRY_POINTS)
    file(READ ${HLSL_FILE} FILE_CONTENT)
    
    set(FOUND_ENTRIES "")
    
    # Look for #pragma vertex directives
    string(REGEX MATCHALL "#pragma[ \t]+vertex[ \t]+([a-zA-Z_][a-zA-Z0-9_]*)" VERTEX_MATCHES ${FILE_CONTENT})
    foreach(MATCH ${VERTEX_MATCHES})
        string(REGEX REPLACE "#pragma[ \t]+vertex[ \t]+([a-zA-Z_][a-zA-Z0-9_]*)" "\\1" ENTRY_POINT ${MATCH})
        list(APPEND FOUND_ENTRIES "${ENTRY_POINT}:vs")
    endforeach()
    
    # Look for #pragma pixel directives
    string(REGEX MATCHALL "#pragma[ \t]+pixel[ \t]+([a-zA-Z_][a-zA-Z0-9_]*)" PIXEL_MATCHES ${FILE_CONTENT})
    foreach(MATCH ${PIXEL_MATCHES})
        string(REGEX REPLACE "#pragma[ \t]+pixel[ \t]+([a-zA-Z_][a-zA-Z0-9_]*)" "\\1" ENTRY_POINT ${MATCH})
        list(APPEND FOUND_ENTRIES "${ENTRY_POINT}:ps")
    endforeach()
    
    # Look for #pragma compute directives
    string(REGEX MATCHALL "#pragma[ \t]+compute[ \t]+([a-zA-Z_][a-zA-Z0-9_]*)" COMPUTE_MATCHES ${FILE_CONTENT})
    foreach(MATCH ${COMPUTE_MATCHES})
        string(REGEX REPLACE "#pragma[ \t]+compute[ \t]+([a-zA-Z_][a-zA-Z0-9_]*)" "\\1" ENTRY_POINT ${MATCH})
        list(APPEND FOUND_ENTRIES "${ENTRY_POINT}:cs")
    endforeach()
    
    # Look for #pragma geometry directives
    string(REGEX MATCHALL "#pragma[ \t]+geometry[ \t]+([a-zA-Z_][a-zA-Z0-9_]*)" GEOMETRY_MATCHES ${FILE_CONTENT})
    foreach(MATCH ${GEOMETRY_MATCHES})
        string(REGEX REPLACE "#pragma[ \t]+geometry[ \t]+([a-zA-Z_][a-zA-Z0-9_]*)" "\\1" ENTRY_POINT ${MATCH})
        list(APPEND FOUND_ENTRIES "${ENTRY_POINT}:gs")
    endforeach()
    
    # Look for #pragma hull directives
    string(REGEX MATCHALL "#pragma[ \t]+hull[ \t]+([a-zA-Z_][a-zA-Z0-9_]*)" HULL_MATCHES ${FILE_CONTENT})
    foreach(MATCH ${HULL_MATCHES})
        string(REGEX REPLACE "#pragma[ \t]+hull[ \t]+([a-zA-Z_][a-zA-Z0-9_]*)" "\\1" ENTRY_POINT ${MATCH})
        list(APPEND FOUND_ENTRIES "${ENTRY_POINT}:hs")
    endforeach()
    
    # Look for #pragma domain directives
    string(REGEX MATCHALL "#pragma[ \t]+domain[ \t]+([a-zA-Z_][a-zA-Z0-9_]*)" DOMAIN_MATCHES ${FILE_CONTENT})
    foreach(MATCH ${DOMAIN_MATCHES})
        string(REGEX REPLACE "#pragma[ \t]+domain[ \t]+([a-zA-Z_][a-zA-Z0-9_]*)" "\\1" ENTRY_POINT ${MATCH})
        list(APPEND FOUND_ENTRIES "${ENTRY_POINT}:ds")
    endforeach()
    
    set(${OUT_ENTRY_POINTS} ${FOUND_ENTRIES} PARENT_SCOPE)
endfunction()

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${SHADER_BINARY_DIR}/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL}
		COMMENT "GLSL ${FILE_NAME}"
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)


foreach(HLSL ${HLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${HLSL} NAME)
    get_filename_component(FILE_NAME_WE ${HLSL} NAME_WE)
    
    # Try to detect entry points using pragma directives
    get_hlsl_entry_points(${HLSL} DETECTED_ENTRIES)
    
    if(DETECTED_ENTRIES)
        # Use detected entry points from pragmas
        foreach(ENTRY_PAIR ${DETECTED_ENTRIES})
            string(REPLACE ":" ";" ENTRY_PROFILE ${ENTRY_PAIR})
            list(GET ENTRY_PROFILE 0 ENTRY_POINT)
            list(GET ENTRY_PROFILE 1 SHADER_PROFILE)
            
			if (SHADER_PROFILE STREQUAL "cs")
				set(SPIRV "${SHADER_BINARY_DIR}/${FILE_NAME_WE}_${ENTRY_POINT}_${SHADER_PROFILE}.spv")
			else()
				set(SPIRV "${SHADER_BINARY_DIR}/${FILE_NAME_WE}_${SHADER_PROFILE}.spv")
            endif()
			
            add_custom_command(
                OUTPUT ${SPIRV}
                COMMAND ${DXC_COMPILER} -E ${ENTRY_POINT} -T ${SHADER_PROFILE}_6_0 -spirv -fspv-target-env=vulkan1.3 -D COMPILING ${HLSL} -Fo ${SPIRV}
                DEPENDS ${HLSL}
                COMMENT "HLSL ${FILE_NAME} (${ENTRY_POINT})"
            )
            list(APPEND SPIRV_BINARY_FILES ${SPIRV})
        endforeach()
    else()
        # Fall back
        if(FILE_NAME MATCHES ".*_vs.hlsl")
            set(ENTRY_POINT "main")
            set(SHADER_PROFILE "vs")
        elseif(FILE_NAME MATCHES ".*_ps.hlsl")
            set(ENTRY_POINT "main")
            set(SHADER_PROFILE "ps")
        elseif(FILE_NAME MATCHES ".*_cs.hlsl")
            set(ENTRY_POINT "main")
            set(SHADER_PROFILE "cs")
        endif()
        
        set(SPIRV "${SHADER_BINARY_DIR}/${FILE_NAME_WE}.spv")
        
        add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${DXC_COMPILER} -E ${ENTRY_POINT} -T ${SHADER_PROFILE}_6_0 -spirv -fspv-target-env=vulkan1.3 -D COMPILING ${HLSL} -Fo ${SPIRV}
            DEPENDS ${HLSL}
            COMMENT "HLSL ${FILE_NAME}"
        )
        list(APPEND SPIRV_BINARY_FILES ${SPIRV})
    endif()
endforeach(HLSL)

set(SHADER_SOURCES ${GLSL_SOURCE_FILES} ${HLSL_SOURCE_FILES})

add_custom_target(shader_binaries ALL DEPENDS ${SPIRV_BINARY_FILES})
add_custom_target(shaders SOURCES ${SHADER_SOURCES})
add_dependencies(shaders shader_binaries)
