cmake_minimum_required(VERSION 3.22.1)
project(RenderRaccoon-Reloaded)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(PROJECT_SOURCE_FILES
        Source/main.cpp
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

# Link Engine Library
add_subdirectory(Engine)
target_link_libraries(${PROJECT_NAME} PRIVATE
        RenderRaccoonEngine
)

# Install glslang via FetchContent
set(GLSLANG_TESTS OFF CACHE BOOL "" FORCE)
set(GLSLANG_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES ON CACHE BOOL "" FORCE)
set(ENABLE_OPT OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        glslang
        GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
        GIT_TAG 16.2.0
)

FetchContent_MakeAvailable(glslang)

# Shader validation target
file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/Assets/Shaders/*.vert"
        "${CMAKE_SOURCE_DIR}/Assets/Shaders/*.frag"
        "${CMAKE_SOURCE_DIR}/Assets/Shaders/*.comp"
        "${CMAKE_SOURCE_DIR}/Assets/Shaders/*.geom"
        "${CMAKE_SOURCE_DIR}/Assets/Shaders/*.tesc"
        "${CMAKE_SOURCE_DIR}/Assets/Shaders/*.tese"
)

set(SHADER_VALIDATION_STAMPS)

foreach(SHADER IN LISTS SHADER_FILES)
    file(RELATIVE_PATH SHADER_REL
            "${CMAKE_SOURCE_DIR}/Assets/Shaders"
            "${SHADER}"
    )

    set(STAMP_FILE
            "${CMAKE_BINARY_DIR}/shader_validation/${SHADER_REL}.stamp"
    )

    get_filename_component(STAMP_DIR "${STAMP_FILE}" DIRECTORY)

    add_custom_command(
            OUTPUT "${STAMP_FILE}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${STAMP_DIR}"
            COMMAND $<TARGET_FILE:glslang-standalone> "${SHADER}"
            COMMAND ${CMAKE_COMMAND} -E touch "${STAMP_FILE}"
            DEPENDS "${SHADER}" glslang-standalone
            COMMENT "Validating shader: ${SHADER_REL}"
            VERBATIM
    )

    list(APPEND SHADER_VALIDATION_STAMPS "${STAMP_FILE}")
endforeach()

add_custom_target(validate_shaders
        DEPENDS ${SHADER_VALIDATION_STAMPS}
)

add_dependencies(${PROJECT_NAME} validate_shaders)

# Copy shaders over to executable after build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Assets/Shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Shaders"
        COMMENT "Copying shaders to output directory"
)