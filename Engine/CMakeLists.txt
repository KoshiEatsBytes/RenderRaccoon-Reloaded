cmake_minimum_required(VERSION 3.22.1)
project(RenderRaccoonEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_SOURCE_FILES
        Source/RR.h
        Source/Engine.h
        Source/Engine.cpp
        Source/Application.h
        Source/Application.cpp
        Source/Input/InputManager.cpp
        Source/Input/InputManager.h
        Source/Graphics/ShaderProgram.h
        Source/Graphics/ShaderProgram.cpp
        Source/Graphics/GraphicsApi.cpp
        Source/Graphics/GraphicsApi.h
        Source/Helpers/ShaderLoader.hpp
        Source/Render/Material.cpp
        Source/Render/Material.h
        Source/Render/Mesh.cpp
        Source/Render/Mesh.h
        Source/Graphics/VertexLayout.h
        Source/Render/RenderQueue.cpp
        Source/Render/RenderQueue.h
)

# Compose application structure
add_library(${PROJECT_NAME} ${PROJECT_SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
        "${CMAKE_CURRENT_SOURCE_DIR}/OpenGL/glew/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/OpenGL/glfw-3.4/include"
)

# Add GLFW library
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/OpenGL/glfw-3.4" "${CMAKE_BINARY_DIR}/glfw_build")

# Add GLEW library
set(BUILD_UTILS OFF CACHE BOOL "utilities" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/OpenGL/glew/build/cmake" "${CMAKE_BINARY_DIR}/glew_build")

target_compile_definitions(${PROJECT_NAME} PUBLIC GLEW_NO_GLU)

set_property(TARGET glew_s PROPERTY FOLDER GLEW)
set_property(TARGET glew PROPERTY FOLDER GLEW)

target_link_libraries(${PROJECT_NAME} PUBLIC
        glfw
        glew_s
)