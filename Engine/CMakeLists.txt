cmake_minimum_required(VERSION 3.22.1)
project(RenderRaccoonEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sets assets dir
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/Assets/")
else()
    set(ASSETS_DIR "./Assets/")
endif()

configure_file(
        ${PROJECT_SOURCE_DIR}/config.h.in
        ${PROJECT_BINARY_DIR}/config.h
)

set(PROJECT_SOURCE_FILES
        Source/RR.h
        Source/Concepts.h
        Source/Types.h
        Source/Common.h
        Source/Engine.h
        Source/Engine.cpp
        Source/Application.h
        Source/Application.cpp
        Source/Helpers/Printer.hpp
        Source/Helpers/Implementations.cpp
        Source/Input/InputManager.cpp
        Source/Input/InputManager.h
        Source/Graphics/ShaderProgram.h
        Source/Graphics/ShaderProgram.cpp
        Source/Graphics/GraphicsApi.cpp
        Source/Graphics/GraphicsApi.h
        Source/Render/Material.cpp
        Source/Render/Material.h
        Source/Render/Mesh.cpp
        Source/Render/Mesh.h
        Source/Graphics/VertexLayout.h
        Source/Render/RenderQueue.cpp
        Source/Render/RenderQueue.h
        Source/Scene/GameObject.cpp
        Source/Scene/GameObject.h
        Source/Scene/Scene.cpp
        Source/Scene/Scene.h
        Source/Scene/Component.cpp
        Source/Scene/Component.h
        Source/Scene/Components/MeshComponent.cpp
        Source/Scene/Components/MeshComponent.h
        Source/Scene/Components/CameraComponent.cpp
        Source/Scene/Components/CameraComponent.h
        Source/Scene/Components/PlayerControllerComponent.cpp
        Source/Scene/Components/PlayerControllerComponent.h
        Source/FileSystem/FileSystem.cpp
        Source/FileSystem/FileSystem.h
        Source/Graphics/Texture.cpp
        Source/Graphics/Texture.h
        Source/Scene/Components/LightComponent.cpp
        Source/Scene/Components/LightComponent.h
        Source/GLTFLib.hpp
)

# Compose application structure
add_library(${PROJECT_NAME} ${PROJECT_SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
        "${CMAKE_CURRENT_SOURCE_DIR}/OpenGL/glew/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/OpenGL/glfw-3.4/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/OpenGL/glm"
        "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/stb_image"
        "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/json"
        "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/cgltf"
)

# Add GLFW library
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/OpenGL/glfw-3.4" "${CMAKE_BINARY_DIR}/glfw_build")

# Add GLEW library
set(BUILD_UTILS OFF CACHE BOOL "utilities" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/OpenGL/glew/build/cmake" "${CMAKE_BINARY_DIR}/glew_build")

# Add GLM library
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/OpenGL/glm" "${CMAKE_BINARY_DIR}/glm_build")

target_compile_definitions(${PROJECT_NAME} PUBLIC GLEW_NO_GLU)

set_property(TARGET glew_s PROPERTY FOLDER GLEW)
set_property(TARGET glew PROPERTY FOLDER GLEW)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_BINARY_DIR})

target_link_libraries(${PROJECT_NAME} PUBLIC
        glfw
        glew_s
        glm
)

# Copies assets over at build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${ASSETS_DIR}
        ${CMAKE_BINARY_DIR}/Assets
)