
#pragma once

#include "RR.h"

class TestObject : public RR::GameObject
{
public:
    TestObject();
    ~TestObject() override = default;

    void Update(float _deltaTime) override;

private:
    RR::Material m_mat;
    std::shared_ptr<RR::Mesh> m_mesh;
};



