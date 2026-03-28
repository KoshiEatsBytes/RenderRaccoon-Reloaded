cmake_minimum_required(VERSION 3.14)
project(RenderRaccoon-Reloaded)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_SOURCE_FILES
        Source/main.cpp
        Source/Helpers/shaderIO.hpp
)

# Compose application structure
add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
        "${CMAKE_CURRENT_SOURCE_DIR}/OpenGL/glfw-3.4/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/OpenGL/glew/include"
)

# Add GLFW library
add_subdirectory(OpenGL/glfw-3.4 "${CMAKE_CURRENT_BINARY_DIR}/glfw_build")

# Add GLEW library
set(BUILD_UTILS OFF CACHE BOOL "utilities" FORCE)
add_subdirectory(OpenGL/glew/build/cmake "${CMAKE_CURRENT_BINARY_DIR}/glew_build")

target_compile_definitions(${PROJECT_NAME} PRIVATE GLEW_NO_GLU)

set_property(TARGET glew_s PROPERTY FOLDER GLEW)
set_property(TARGET glew PROPERTY FOLDER GLEW)

# Link all OpenGL libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        glew_s
)

# Add shader support
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Shaders"
        COMMENT "Copying shaders to output directory"
)