
#pragma once

// Helpers
#include "Helpers/Printer.hpp"

// Engine
#include "ApplicationManager.h"
#include "Engine.h"
#include "Helpers/Types.h"
#include "Helpers/TypeInfo.h"
#include "Input/InputManager.h"
#include "FileSystem/FileSystem.h"

// Physics
#include "Physics/PhysicsManager.h"
#include "Physics/Collider.h"
#include "Physics/RigidBody.h"
#include "Physics/KinematicCharacterController.h"

// Graphics
#include "Graphics/ShaderProgram.h"
#include "Graphics/GraphicsAPI.h"
#include "Graphics/VertexLayout.h"
#include "Graphics/Texture.h"
#include "Render/Material.h"
#include "Render/Mesh.h"
#include "Render/RenderQueue.h"

// Scene
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/Component.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/PlayerControllerComponent.h"
#include "Scene/Components/LightComponent.h"
#include "Scene/Components/AnimationComponent.h"
#include "Scene/Components/PhysicsComponent.h"
#include "Scene/Components/ColliderComponent.h"
#include "Scene/Components/AudioListenerComponent.h"
#include "Scene/Components/AudioSourceComponent.h"

// Audio
#include "Audio/AudioManager.h"
