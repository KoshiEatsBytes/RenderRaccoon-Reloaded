
#pragma once

#include "RR.h"

class TestObject : public RR::GameObject
{
public:
    TestObject();
    ~TestObject() override = default;

    void Update(float _deltaTime) override;

private:
};



