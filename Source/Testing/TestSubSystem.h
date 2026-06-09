
#pragma once
#include "ISubSystem.h"
#include "Helpers/TypeInfo.h"

class TestSubSystem : public RR::ISubSystem
{
public:
    SUBSYSTEM(TestSubSystem, RR::ISubSystem)

    TestSubSystem() = default;
    ~TestSubSystem() override = default;

protected:
    bool Init() override;
    void Update(float _deltaTime) override;
};

