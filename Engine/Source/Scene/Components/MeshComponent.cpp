
#include "MeshComponent.h"
#include "Render/Material.h"
#include "Render/Mesh.h"
#include "Render/RenderQueue.h"
#include "Scene/GameObject.h"
#include "Engine.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    MeshComponent::MeshComponent()
    = default;

    MeshComponent::MeshComponent(const std::shared_ptr<Material>& _material, const std::shared_ptr<Mesh>& _mesh)
        : m_material(_material), m_mesh(_mesh)
    {
    }

    MeshComponent::~MeshComponent()
    = default;

    void MeshComponent::Init()
    {
        Component::Init();
    }

    void MeshComponent::Update(float _deltaTime)
    {
        if (!m_material || !m_mesh) return;

        // Fires a render queue for this mesh
        RenderCommand command;
        command.material = m_material.get();
        command.mesh = m_mesh.get();
        command.modelMatrix = GetOwner()->GetWorldTransform();
        command.color = m_color;

        Engine::GetInstance().GetRenderQueue().Submit(command);
    }

    // Get/Sets --------------------------------------------------------------------------------------------------------

    void MeshComponent::SetMaterial(const std::shared_ptr<Material>& _material)
    {
        m_material = _material;
    }

    void MeshComponent::SetMesh(const std::shared_ptr<Mesh>& _mesh)
    {
        m_mesh = _mesh;
    }

    void MeshComponent::SetColor(const vec3& _color)
    {
        m_color = _color;
    }
}
