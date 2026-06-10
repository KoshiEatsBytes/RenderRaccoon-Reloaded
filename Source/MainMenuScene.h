
#pragma once
#include <RR.h>

class MainMenuScene : public RR::Scene
{
public:
    explicit MainMenuScene();
    ~MainMenuScene() override = default;

protected:
    bool Init() override;
    void PreUpdate(float _deltaTime) override;
    void Update(float _deltaTime) override;
    void LateUpdate(float _deltaTime) override;
    void Destroy() override;
};



