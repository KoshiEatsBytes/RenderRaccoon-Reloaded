
#pragma once
#include <memory>
#include <string>

#include "Helpers/Types.h"

namespace RR
{
    class Mesh;
    class Material;

    class Skybox
    {
    public:
        Skybox();
        ~Skybox();

        bool Load(const std::string& _materialPath);

        void SetHorizonColor(const vec3& _color);
        void SetZenithColor (const vec3& _color);
        void SetSun(const vec3& _direction, float _size);

        void SubmitForDraw();

    private:
        std::shared_ptr<Mesh>     m_mesh;
        std::shared_ptr<Material> m_material;
    };
}