
#include "Skybox.h"
#include <GL/glew.h>
#include "Engine.h"
#include "Render/Mesh.h"
#include "Render/Material.h"
#include "Render/RenderQueue.h"
#include "Graphics/VertexLayout.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    Skybox::Skybox()
    = default;

    Skybox::~Skybox()
    = default;

    bool Skybox::Load(const std::string& _materialPath)
    {
        // Assemble skybox mesh as a giant triangle
        VertexLayout skyLayout;

        skyLayout.elements.push_back(
            {0, 2, GL_FLOAT, 0}
            );
        skyLayout.stride = 2 * sizeof(float);

        const std::vector<float> verts = {
            -1.f, -1.f, 3.f, -1.f, -1.f, 3.f
        };

        // Assemble mesh and material
        m_mesh     = std::make_shared<Mesh>(skyLayout, verts);
        m_material = Material::Load(_materialPath);

        return m_material != nullptr;
    }

    void Skybox::SetHorizonColor(const vec3& _color)
    {
        if (!m_material) return;

        m_material->SetParam("uSkyHorizon", _color);
    }

    void Skybox::SetZenithColor(const vec3& _color)
    {
        if (!m_material) return;

        m_material->SetParam("uSkyZenith", _color);
    }

    void Skybox::SetSun(const vec3& _direction, float _size)
    {
        if (!m_material) return;

        m_material->SetParam("uSunDir", glm::normalize(_direction));
        m_material->SetParam("uSunSize", _size);
    }

    void Skybox::SubmitForDraw()
    {
        if (!m_mesh || !m_material) return;

        // Submit sky for rendering
        RenderCommand skyCmd;
        skyCmd.mesh        = m_mesh.get();
        skyCmd.material    = m_material.get();
        skyCmd.modelMatrix = mat4(1.0f);
        skyCmd.color       = vec3(1.0f);
        Engine::GetInstance().GetRenderQueue().Submit(skyCmd);
    }
}