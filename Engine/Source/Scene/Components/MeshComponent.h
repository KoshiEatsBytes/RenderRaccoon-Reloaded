
#pragma once
#include <memory>

#include "Scene/Component.h"

namespace RR
{
    class Material;
    class Mesh;

    class MeshComponent : public Component
    {
    public:
        COMPONENT(MeshComponent)

        MeshComponent(const std::shared_ptr<Material>& _material,
            const std::shared_ptr<Mesh>& _mesh);
        ~MeshComponent() override;

        void Init() override;
        void Update(float _deltaTime) override;

    private:
        std::shared_ptr<Material> m_material;
        std::shared_ptr<Mesh> m_mesh;
    };
}

