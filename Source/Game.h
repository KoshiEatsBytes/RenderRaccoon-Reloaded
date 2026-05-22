#pragma once

#include <RR.h>

class Game : public RR::Application
{
public:
    Game();
    ~Game();

    bool Init()                         override;
    void Update(const float &deltaTime) override;
    void Destroy()                      override;

private:
    RR::Material m_mat;
};


