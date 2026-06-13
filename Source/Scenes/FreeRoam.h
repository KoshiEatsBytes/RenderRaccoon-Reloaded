
#pragma once
#include "RR.h"

class FreeRoam : public RR::Scene
{
public:
    explicit FreeRoam();
    ~FreeRoam() override;

protected:
    bool Init() override;
    void PreUpdate(float _deltaTime) override;
    void Update(float _deltaTime) override;
    void LateUpdate(float _deltaTime) override;
    void Destroy() override;

private:
    RR::GameObject* m_cam = nullptr;
};

