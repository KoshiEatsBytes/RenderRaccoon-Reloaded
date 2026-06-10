cmake_minimum_required(VERSION 3.22.1)
project(RenderRaccoonEngine)

find_package(OpenMP REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

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
        Source/Engine.h
        Source/Engine.cpp
        Source/ISubSystem.h
        Source/ISubSystem.cpp
        Source/ApplicationManager.h
        Source/ApplicationManager.cpp
        Source/Helpers/Concepts.h
        Source/Helpers/Types.h
        Source/Helpers/Common.h
        Source/Helpers/Printer.hpp
        Source/Helpers/GLTFLib.hpp
        Source/Helpers/Implementations/Implementations.cpp
        Source/Helpers/ApplicationData.h
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
        Source/Scene/Components/AnimationComponent.cpp
        Source/Scene/Components/AnimationComponent.h
        Source/Helpers/AnimationStructs.h
        Source/Physics/PhysicsManager.cpp
        Source/Physics/PhysicsManager.h
        Source/Physics/Collider.cpp
        Source/Physics/Collider.h
        Source/Physics/RigidBody.cpp
        Source/Physics/RigidBody.h
        Source/Scene/Components/PhysicsComponent.cpp
        Source/Scene/Components/PhysicsComponent.h
        Source/Physics/BtConv.hpp
        Source/Scene/Components/ColliderComponent.cpp
        Source/Scene/Components/ColliderComponent.h
        Source/Physics/KinematicCharacterController.cpp
        Source/Physics/KinematicCharacterController.h
        Source/Helpers/TypeInfo.h
        Source/Audio/AudioManager.cpp
        Source/Audio/AudioManager.h
        Source/Scene/Components/AudioSourceComponent.cpp
        Source/Scene/Components/AudioSourceComponent.h
        Source/Scene/Components/AudioListenerComponent.cpp
        Source/Scene/Components/AudioListenerComponent.h
        Source/Audio/Voice/AudioClip.h
        Source/Audio/Voice/AudioVoice.cpp
        Source/Audio/Voice/AudioVoice.h
        Source/Audio/SpatialAudio.cpp
        Source/Audio/SpatialAudio.h
        Source/Audio/StaticAudio.cpp
        Source/Audio/StaticAudio.h
        Source/Audio/AudioChannel.cpp
        Source/Audio/AudioChannel.h
        Source/Audio/Tracker.h
        Source/Audio/ComponentAudioTracker.h
        Source/Audio/ComponentAudioTracker.cpp
        Source/Audio/ManagerAudioTracker.h
        Source/Audio/ManagerAudioTracker.cpp
        Source/ShowCase.cpp
        Source/ShowCase.h
        Source/Render/MeshData.h
        Source/Render/Voxels/ChunkMesher.cpp
        Source/Render/Voxels/ChunkMesher.h
        Source/Benchmark/BenchmarkData.cpp
        Source/Benchmark/BenchmarkData.h
        Source/Benchmark/BenchmarkSubSystem.cpp
        Source/Benchmark/BenchmarkSubSystem.h
        Source/Benchmark/BenchmarkParser.hpp
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
        "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/bullet3-3.25/src"
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

# add bullet library
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/bullet3-3.25" "${CMAKE_BINARY_DIR}/bullet_build")

# Enable Bullet's multithreaded solver/dispatcher
foreach(BT_TARGET LinearMath BulletCollision BulletDynamics)
    target_compile_definitions(${BT_TARGET} PUBLIC BT_THREADSAFE=1 BT_USE_OPENMP=1)
    target_link_libraries(${BT_TARGET} PUBLIC OpenMP::OpenMP_CXX)
endforeach()

# Include mini-audio library
add_library(miniaudio STATIC "${CMAKE_CURRENT_SOURCE_DIR}/Source/Helpers/Implementations/miniaudio_impl.c")

target_include_directories(miniaudio PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/miniaudio")

if (WIN32)
    target_link_libraries(miniaudio PUBLIC winmm ole32)
elseif (UNIX AND NOT APPLE)
    target_link_libraries(miniaudio PUBLIC dl pthread m)
elseif(APPLE)
    find_library(COREAUDIO_FRAMEWORK CoreAudio)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
    find_library(AUDIOUNIT_FRAMEWORK AudioUnit)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
    target_link_libraries(miniaudio PUBLIC
            ${COREAUDIO_FRAMEWORK}
            ${AUDIOUNIT_FRAMEWORK}
            ${AUDIOTOOLBOX_FRAMEWORK}
            ${COREFOUNDATION_FRAMEWORK}
    )
endif()

# Add Dear ImGui library
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui")

add_library(imgui STATIC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
        ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)

target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${IMGUI_DIR}/misc/cpp
)

target_link_libraries(imgui PUBLIC glfw)

# link libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
        glfw
        glew_s
        glm
        BulletDynamics
        BulletCollision
        LinearMath
        miniaudio
        imgui
)

# Copies assets over at build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets/
        ${CMAKE_BINARY_DIR}/Assets
)
