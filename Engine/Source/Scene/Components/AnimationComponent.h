
#pragma once
#include "Scene/Component.h"
#include "AnimationStructs.h"

namespace RR
{
    class AnimationComponent : public Component
    {
    public:
        COMPONENT(AnimationComponent);

        AnimationComponent();
        ~AnimationComponent() override;

        void Init() override;
        void Update(float _deltaTime) override;

        void Play(const std::string& _name, bool loop = true);
        void RegisterClip(const std::string& _name, const std::shared_ptr<AnimationClip>& _clip);
        void SetClip(AnimationClip* _clip);

    private:
        void BuildBindings();

        vec3 Interpolate(const std::vector<KeyFrameVec3>& _keys, float _time);
        quat Interpolate(const std::vector<KeyFrameQuat>& _keys, float _time);

        AnimationClip* m_clip = nullptr;

        bool m_looping = true;
        bool m_isPlaying = false;
        float m_time = 0.0f;

        std::unordered_map<std::string, std::shared_ptr<AnimationClip>> m_clips;
        std::unordered_map<GameObject*, std::unique_ptr<ObjectBinding>> m_bindings;
    };
}