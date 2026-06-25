
#include "Skybox.h"
#include <GL/glew.h>
#include "Engine.h"
#include "Render/Mesh.h"
#include "Render/Material.h"
#include "Render/RenderQueue.h"
#include "Graphics/VertexLayout.h"

namespace CLOUD
{
    using uInt32 = std::uint32_t;

    // World units per cloud cell, thickness and radious
    constexpr float kCloudCell   = 12.0f;
    constexpr float kCloudThick  = 4.0f;
    constexpr int   kCloudRadius = 40;

    uInt32 cHash(int _x, int _z, uInt32 _seed)
    {
        // salted cloud hash
        uInt32 hash = static_cast<uInt32>(_x)*374761393u +
                      static_cast<uInt32>(_z)*668265263u +
                      _seed*362437u;

        hash = (hash ^ (hash >> 13)) * 1274126177u;

        return hash ^ (hash >> 16);
    }

    float cVal(int _x, int _z, uInt32 _seed)
    {
        return (cHash(_x, _z, _seed) & 0xFFFFFF) / static_cast<float>(0xFFFFFF);
    }

    // value noise over cloud cells
    float cNoise(float _x, float _z, uInt32 _seed)
    {
        const int xi = static_cast<int>(std::floor(_x));
        const int zi = static_cast<int>(std::floor(_z));

        float fx = _x - xi;
        float fz = _z - zi;

        fx = fx*fx*(3-2*fx);
        fz = fz*fz*(3-2*fz);

        return glm::mix(
             glm::mix(cVal(xi, zi, _seed),   cVal(xi + 1, zi, _seed), fx),
             glm::mix(cVal(xi, zi + 1, _seed), cVal(xi + 1, zi + 1, _seed), fx),
             fz);
    }

    bool CloudAt(int _cx, int _cz, uInt32 _seed, float _coverage, float _scale)
    {
        return cNoise(_cx / _scale, _cz / _scale, _seed) > _coverage;
    }

}

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