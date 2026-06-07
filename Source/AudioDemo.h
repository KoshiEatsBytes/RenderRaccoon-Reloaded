#pragma once
#include <unordered_map>

#include <RR.h>
#include "Player.h"

namespace RR { class AudioSourceComponent; }

class AudioDemo : public RR::Scene
{
public:
    AudioDemo();
    ~AudioDemo() override;

protected:
    bool Init()                       override;
    void PreUpdate(float _deltaTime)  override;
    void Update(float _deltaTime)     override;
    void LateUpdate(float _deltaTime) override;
    void Destroy()                    override;

private:
    bool JustPressed(int _key);

    RR::GameObject*           m_emitter       = nullptr;
    RR::AudioSourceComponent* m_emitterSource = nullptr;
    float m_time = 0.0f;
    std::unordered_map<int, bool> m_prevKeys;

};
