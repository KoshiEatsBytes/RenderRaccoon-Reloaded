#pragma once

#include <RR.h>
#include <memory>

class Game : public RR::Application
{
public:
    Game();
    ~Game() override;

    bool Init()                         override;
    void Update(float _deltaTime)       override;
    void Destroy()                      override;

private:
    RR::Material m_mat;
    std::unique_ptr<RR::Mesh> m_mesh;
    
    float m_offsetX {0.0f};
    float m_offsetY {0.0f};
};


