
#pragma once
#include <memory>
#include <string>

#include "Helpers/Types.h"

namespace RR
{
    class Mesh;
    class Material;

    class VoxelSkybox
    {
    public:
        VoxelSkybox();
        ~VoxelSkybox();

        bool Load(const std::string& _skyMatPath, const std::string& _cloudMatPath);
        void UpdateClouds(const vec3& _cameraPos, float _dt);

        // sky sets
        void SetHorizonColor(const vec3& _color);
        void SetZenithColor (const vec3& _color);
        void SetSun(const vec3& _direction, float _size);

        // cloud sets
        void SetClouds       (uInt32 _seed, int _height, float _scale, float _cov);
        void SetCloudSeed    (uInt32 _seed);
        void SetCloudHeight  (int _height);
        void SetCloudScale   (float _scale);
        void SetCloudCoverage(float _cov);
        void SetCloudColor   (const vec3& _color);
        void SetCloudFade    (float _fadeStart, float _fadeEnd);

        // Wind sets
        void SetWind(const vec2& _dir, float _speed);
        void SetWindDir(const vec2& _dir);
        void SetWindSpeed(float _speed);

        void SubmitSkyForDraw();
        void SubmitCloudsForDraw();

    private:
        void BuildClouds(int _centreX, int _centreZ);

        std::shared_ptr<Mesh>     m_skyMesh;
        std::shared_ptr<Material> m_skyMat;

        // clouds
        std::shared_ptr<Mesh>     m_cloudMesh;
        std::shared_ptr<Material> m_cloudMat;

        int m_cloudBuildX = 0;
        int m_cloudBuildZ = 0;

        int    m_cloudHeight   = 230;
        float  m_cloudScale    = 1.5f;
        float  m_cloudCoverage = 0.625f;
        uInt32 m_cloudSeed     = 42;

        // Wing
        vec2  m_windDir   = glm::normalize(vec2(1.0f, 0.35f));
        vec2  m_windOff   = vec2(0.0f);
        float m_windSpeed = 2.0f;
        float m_time      = 0.0f;
    };
}