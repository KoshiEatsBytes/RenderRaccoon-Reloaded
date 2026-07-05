cmake_minimum_required(VERSION 3.22.1)
project(RenderRaccoon-Reloaded)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(PROJECT_SOURCE_FILES
        Source/main.cpp
        Source/Components/FreeCameraComponent.cpp
        Source/Components/FreeCameraComponent.h
        Source/WorldGen/Noise.hpp
        Source/WorldGen/WorldGen.hpp
        Source/WorldGen/WorldGenConfig.h
        Source/WorldGen/Biome.hpp
        Source/WorldGen/BiomeID.h
        Source/WorldGen/BiomeMap.hpp
        Source/WorldGen/Vegetation.hpp
        Source/Scenes/Base/VoxelScene.cpp
        Source/Scenes/Base/VoxelScene.h
        Source/Scenes/BenchmarkScene.h
        Source/Scenes/BenchmarkScene.cpp
        Source/Scenes/FreeRoamScene.cpp
        Source/Scenes/FreeRoamScene.h
        Source/Scenes/MainMenuScene.cpp
        Source/Scenes/MainMenuScene.h
        Source/Benchmark/CameraPath.hpp
        Source/WorldGen/SurfaceLOD.hpp
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_FILES})

# make sure wip files are always copied over
add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets ${CMAKE_BINARY_DIR}/Assets
        COMMENT "Syncing Assets to build dir")
add_dependencies(${PROJECT_NAME} copy_assets)

target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

# Link Engine Library
add_subdirectory(Engine)
target_link_libraries(${PROJECT_NAME} PRIVATE
        RenderRaccoonEngine
)

# Distribution, copies assets and needed folders
set(DIST_DIR "${CMAKE_BINARY_DIR}/dist" CACHE PATH "Folder the builds moves packages into")

# guard folder creation so it doesnt wipe everything in its path
if(NOT DIST_DIR)
    message(FATAL_ERROR "DIST_DIR is empty, directory is invalid!")
endif()

get_filename_component(_distAbs "${DIST_DIR}"         ABSOLUTE)
get_filename_component(_binAbs  "${CMAKE_BINARY_DIR}" ABSOLUTE)
get_filename_component(_srcAbs  "${CMAKE_SOURCE_DIR}" ABSOLUTE)
string(TOLOWER "${_distAbs}/" _distLow)
string(TOLOWER "${_binAbs}/"  _binLow)
string(TOLOWER "${_srcAbs}/"  _srcLow)

string(FIND "${_binLow}" "${_distLow}" _binHit)
string(FIND "${_srcLow}" "${_distLow}" _srcHit)
if(_binHit EQUAL 0)
    message(FATAL_ERROR "DIST_DIR ('${DIST_DIR}') is the build directory, select a different directory")
endif()
if(_srcHit EQUAL 0)
    message(FATAL_ERROR "DIST_DIR ('${DIST_DIR}') is the build directory, select a different directory")
endif()

add_custom_target(dist
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${DIST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}>" "${DIST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/Assets" "${DIST_DIR}/Assets"
        VERBATIM
        COMMENT "Packing build to -> ${DIST_DIR}")

# build the exe before packaging it
add_dependencies(dist ${PROJECT_NAME})

# non static profiles, drop needed dlls next to executable
option(DIST_BUNDLE_DLLS "Copy mingw runtime dlls into the dist folder" OFF)
if(DIST_BUNDLE_DLLS AND MINGW)
    get_filename_component(MINGW_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
    add_custom_command(TARGET dist POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${MINGW_BIN}/libgcc_s_seh-1.dll"
                "${MINGW_BIN}/libstdc++-6.dll"
                "${MINGW_BIN}/libwinpthread-1.dll"
                "${DIST_DIR}/"
            VERBATIM
            COMMENT "Bundling mingw runtime dlls")
endif()

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
