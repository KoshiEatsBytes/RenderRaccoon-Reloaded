
#include "VoxelSkybox.h"
#include <GL/glew.h>
#include "Engine.h"
#include "Render/Mesh.h"
#include "Render/Material.h"
#include "Render/RenderQueue.h"
#include "Graphics/VertexLayout.h"

namespace CLOUD
{
    // World units per cloud cell, thickness and radius
    constexpr float kCloudCell   = 12.0f;
    constexpr float kCloudThick  = 4.0f;
    constexpr int   kCloudRadius = 64;
    constexpr int   kRebuildStep = 8;

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

    VoxelSkybox::VoxelSkybox()
    = default;

    VoxelSkybox::~VoxelSkybox()
    = default;

    bool VoxelSkybox::Load(const std::string& _skyMatPath, const std::string& _cloudMatPath)
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
        m_skyMesh = std::make_shared<Mesh>(skyLayout, verts);
        m_skyMat  = Material::Load(_skyMatPath);

        // Assemble clouds
        m_cloudMat = Material::Load(_cloudMatPath);

        return m_skyMat != nullptr;
    }

    void VoxelSkybox::UpdateClouds(const vec3& _cameraPos, float _dt)
    {
        using namespace CLOUD;
        m_time   += _dt;
        m_windOff = m_windDir * (m_windSpeed * m_time);

        // build the centre around windoffset
        const int camX = static_cast<int>(std::floor((_cameraPos.x - m_windOff.x) / kCloudCell));
        const int camZ = static_cast<int>(std::floor((_cameraPos.z - m_windOff.y) / kCloudCell));

        const bool firstBuild = !m_cloudMesh;
        const int  moved  = std::max(std::abs(camX - m_cloudBuildX),
                                     std::abs(camZ - m_cloudBuildZ));

        if (firstBuild || moved >= kRebuildStep)
        {
            // Re centre on cam
            BuildClouds(camX, camZ);
            m_cloudBuildX = camX;
            m_cloudBuildZ = camZ;
        }
    }

    void VoxelSkybox::SetHorizonColor(const vec3& _color)
    {
        if (m_skyMat)   m_skyMat->SetParam("uSkyHorizon", _color);
        if (m_cloudMat) m_cloudMat->SetParam("uHorizonColor", _color);
    }

    void VoxelSkybox::SetZenithColor(const vec3& _color)
    {
        if (m_skyMat)   m_skyMat->SetParam("uSkyZenith", _color);
        if (m_cloudMat) m_cloudMat->SetParam("uZenithColor", _color);
    }

    void VoxelSkybox::SetSun(const vec3& _direction, float _size)
    {
        if (!m_skyMat) return;

        m_skyMat->SetParam("uSunDir", glm::normalize(_direction));
        m_skyMat->SetParam("uSunSize", _size);
    }

    void VoxelSkybox::SetClouds(uInt32 _seed, int _height, float _scale, float _cov)
    {
        m_cloudSeed     = _seed;
        m_cloudHeight   = _height;
        m_cloudScale    = _scale;
        m_cloudCoverage = _cov;
    }

    void VoxelSkybox::SetCloudSeed(uInt32 _seed)
    {
        m_cloudSeed = _seed;
    }

    void VoxelSkybox::SetCloudHeight(int _height)
    {
        m_cloudHeight = _height;
    }

    void VoxelSkybox::SetCloudScale(float _scale)
    {
        m_cloudScale = _scale;
    }

    void VoxelSkybox::SetCloudCoverage(float _cov)
    {
        m_cloudCoverage = _cov;
    }

    void VoxelSkybox::SetCloudColor(const vec3& _color)
    {
        if (!m_cloudMat) return;

        m_cloudMat->SetParam("uCloudColor", _color);
    }

    void VoxelSkybox::SetCloudFade(float _fadeStart, float _fadeEnd)
    {
        using namespace CLOUD;

        // Preset cloud fading point
        const float cloudFadeEnd   = kCloudRadius * kCloudCell * _fadeStart;
        const float cloudFadeStart = cloudFadeEnd * _fadeEnd;
        m_cloudMat->SetParam("uFogStart", cloudFadeStart);
        m_cloudMat->SetParam("uFogEnd",   cloudFadeEnd);
    }

    void VoxelSkybox::SetWind(const vec2& _dir, float _speed)
    {
        m_windDir   = _dir;
        m_windSpeed = _speed;
    }

    void VoxelSkybox::SetWindDir(const vec2& _dir)
    {
        m_windDir = _dir;
    }

    void VoxelSkybox::SetWindSpeed(float _speed)
    {
        m_windSpeed = _speed;
    }

    void VoxelSkybox::SubmitSkyForDraw()
    {
        if (!m_skyMesh || !m_skyMat) return;

        // Submit sky for rendering
        RenderCommand skyCmd;
        skyCmd.mesh        = m_skyMesh.get();
        skyCmd.material    = m_skyMat.get();
        skyCmd.modelMatrix = mat4(1.0f);
        skyCmd.color       = vec3(1.0f);
        Engine::GetInstance().GetRenderQueue().Submit(skyCmd);
    }

    void VoxelSkybox::SubmitCloudsForDraw()
    {
        if (!m_cloudMesh || !m_cloudMat) return;

        RenderCommand cldCmd;
        cldCmd.mesh     = m_cloudMesh.get();
        cldCmd.material = m_cloudMat.get();
        cldCmd.color    = vec3(1.0f);

        // Applies wind drift to clouds translation
        cldCmd.modelMatrix = glm::translate(
                          mat4(1.0f),
                           vec3(m_windOff.x,
                           0.0f,
                           m_windOff.y));

        Engine::GetInstance().GetRenderQueue().Submit(cldCmd);
    }

    void VoxelSkybox::BuildClouds(int _centreX, int _centreZ)
    {
        using namespace CLOUD;

        std::vector<float> verts;

        // Reserve vec to prevent mid-runtime allocations
        constexpr int cellCount = (2 * kCloudRadius + 1) * (2 * kCloudRadius + 1);
        verts.reserve(static_cast<size_t>(cellCount) * 6 * 6 * 4);

        const float  scale    = m_cloudScale;
        const float  coverage = m_cloudCoverage;
        const uInt32 seed     = m_cloudSeed;

        // helper lambda to build cloud quads
        auto buildQuad = [&](vec3 _a, vec3 _b, vec3 _c, vec3 _d, float _shade)
        {
            vec3 tris[6] = {_a, _b, _c,
                            _a, _c, _d };

            for (vec3 tri : tris)
            {
                verts.insert(verts.end(), {
                    tri.x, tri.y, tri.z, _shade}
                );
            }
        };

        auto neighbourAt = [&](int _nx, int _nz) -> bool {
            return CloudAt(_nx, _nz, seed, coverage, scale);
        };

        // cloud position and thicknes
        const float y0 = static_cast<float>(m_cloudHeight);
        const float y1 = static_cast<float>(m_cloudHeight) + kCloudThick;

        for (int cz = _centreZ - kCloudRadius; cz <= _centreZ + kCloudRadius; ++cz)
        {
            for (int cx = _centreX - kCloudRadius; cx <= _centreX + kCloudRadius; ++cx)
            {
                // discard current cell if not cloud
                if (!CloudAt(cx, cz, seed, coverage, scale)) continue;

                // Assemble cloud aabb
                const float x0 = cx * kCloudCell;
                const float x1 = x0 + kCloudCell;
                const float z0 = cz * kCloudCell;
                const float z1 = z0 + kCloudCell;

                // top and cloud bottom
                buildQuad({x0, y1, z0},{x0, y1, z1},{x1, y1, z1},{x1, y1, z0}, 1.00f);
                buildQuad({x0, y0, z1},{x0, y0, z0},{x1, y0, z0},{x1, y0, z1}, 0.70f);

                // sides only if the neighbours are empty
                // east
                if (!neighbourAt(cx + 1, cz))
                {
                    buildQuad({x1,y0,z0},{x1,y1,z0},{x1,y1,z1},{x1,y0,z1}, 0.85f);
                }
                // west
                if (!neighbourAt(cx - 1, cz))
                {
                    buildQuad({x0,y0,z1},{x0,y1,z1},{x0,y1,z0},{x0,y0,z0}, 0.85f);
                }
                // south
                if (!neighbourAt(cx, cz + 1))
                {
                    buildQuad({x1,y0,z1},{x1,y1,z1},{x0,y1,z1},{x0,y0,z1}, 0.80f);
                }
                // north
                if (!neighbourAt(cx, cz - 1))
                {
                    buildQuad({x0,y0,z0},{x0,y1,z0},{x1,y1,z0},{x1,y0,z0}, 0.80f);
                }
            }
        }

        VertexLayout cloudLayout;
        // Pos
        cloudLayout.elements.push_back({0, 3, GL_FLOAT, 0});
        // Shade
        cloudLayout.elements.push_back({1, 1, GL_FLOAT, 3 * sizeof(float)});
        // stride
        cloudLayout.stride = 4 * sizeof(float);

        if (!verts.empty())
        {
            m_cloudMesh = std::make_shared<Mesh>(cloudLayout, verts);
            return;
        }

        //discard if no clouds
        m_cloudMesh = nullptr;
    }
}
