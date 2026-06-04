
#pragma once

#include "RR.h"

class TestObject : public RR::GameObject
{
public:
    GAMEOBJECT(TestObject, RR::GameObject);

    TestObject();
    ~TestObject() override = default;

    void Update(float _deltaTime) override;

private:
};



