
#include "AnimationComponent.h"

#include "Helpers/Printer.hpp"
#include "Scene/GameObject.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    AnimationComponent::AnimationComponent()
    = default;

    AnimationComponent::~AnimationComponent()
    = default;

    void AnimationComponent::Update(float _deltaTime)
    {
        if (!m_clip) return;
        if (!m_isPlaying) return;

        m_time += _deltaTime;

        // check duration
        if (m_time > m_clip->duration)
        {
            // keep repeating
            if (m_looping)
            {
                m_time = std::fmod(m_time, m_clip->duration);
            }
            else
            {
                // once its done reset
                m_time = 0.0f;
                m_isPlaying = false;
                return;
            }
        }

        // Integrate over all bindings
        for (auto& [obj, binding] : m_bindings)
        {
            auto& trackIndices = binding->trackIndices;

            // Extract indices and interpolate their pos
            for (auto& i : trackIndices)
            {
                auto& track = m_clip->tracks[i];

                // Pos
                if (!track.positions.empty())
                {
                    auto pos = Interpolate(track.positions, m_time);
                    obj->SetPosition(pos);
                }
                // Rot
                if (!track.rotations.empty())
                {
                    auto rot = Interpolate(track.rotations, m_time);
                    obj->SetRotation(rot);
                }
                // Scale
                if (!track.scales.empty())
                {
                    auto scale = Interpolate(track.scales, m_time);
                    obj->SetScale(scale);
                }
            }
        }

    }

    void AnimationComponent::Play(const std::string& _name, bool loop)
    {
        // If already on clip, restart
        if (m_clip && m_clip->name == _name)
        {
            m_time = 0.0f;
            m_isPlaying = true;
            m_looping = loop;
        }
        else // Otherwise start from registered clip
        {
            auto it = m_clips.find(_name);
            if (it != m_clips.end())
            {
                SetClip(it->second.get());
                m_time = 0.0f;
                m_isPlaying = true;
                m_looping = loop;
            }
        }
    }

    void AnimationComponent::RegisterClip(const std::string& _name, const std::shared_ptr<AnimationClip>& _clip)
    {
        m_clips[_name] = _clip;
    }

    void AnimationComponent::SetClip(AnimationClip* _clip)
    {
        m_clip = _clip;
        BuildBindings();
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

    void AnimationComponent::BuildBindings()
    {
        m_bindings.clear();
        if (!m_clip)
        {
            Warn("[ANIMATION - BINDING] Tried binding INVALID animation to animation component");
            return;
        }

        // Iterate over clip tracks
        for (sizeT i = 0; i < m_clip->tracks.size(); i++)
        {
            auto& track = m_clip->tracks[i];
            auto targetObject = m_owner->GetChildByName(track.targetName);

            // if the clips target object is valid, bind object to clip
            if (targetObject)
            {
                auto it = m_bindings.find(targetObject);
                if (it != m_bindings.end())
                {
                    it->second->trackIndices.push_back(i);
                }
                else
                {
                    auto binding = std::make_unique<ObjectBinding>();
                    binding->object = targetObject;
                    binding->trackIndices.push_back(i);
                    m_bindings.emplace(targetObject, std::move(binding));
                }
            }
        }
    }

    vec3 AnimationComponent::Interpolate(const std::vector<KeyFrameVec3> &_keys, float _time)
    {
        // nothing to do, return 0
        if (_keys.empty()) return vec3(0.0f);

        // 1 key return only key value
        if (_keys.size() == 1)
        {
            return _keys[0].value;
        }
        // outside timeframe edge-cases return either first or last key
        if (_time <= _keys.front().timeStamp)
        {
            return _keys.front().value;
        }
        if (_time >= _keys.back().timeStamp)
        {
            return _keys.back().value;
        }

        sizeT i0 = 0;
        sizeT i1 = 0;

        // Find key timeframe containing the timestamp
        for (sizeT i = 1; i < _keys.size(); i++)
        {
            if (_time <= _keys[i].timeStamp)
            {
                i1 = i;
                break;
            }
        }

        i0 = i1 > 0 ? i1 - 1 : 0;

        // blend the two values
        if (_time >= _keys[i0].timeStamp && _time <= _keys[i1].timeStamp)
        {
            float deltaTime = _keys[i1].timeStamp - _keys[i0].timeStamp;
            float k = (_time - _keys[i0].timeStamp) / deltaTime;

            return glm::mix(_keys[i0].value, _keys[i1].value, k);
        }

        return _keys.back().value;
    }

    quat AnimationComponent::Interpolate(const std::vector<KeyFrameQuat> &_keys, float _time)
    {
        // nothing to do, return 0
        if (_keys.empty()) return vec3(0.0f);

        // 1 key return only key value
        if (_keys.size() == 1)
        {
            return _keys[0].value;
        }
        // outside timeframe edge-cases return either first or last key
        if (_time <= _keys.front().timeStamp)
        {
            return _keys.front().value;
        }
        if (_time >= _keys.back().timeStamp)
        {
            return _keys.back().value;
        }

        sizeT i0 = 0;
        sizeT i1 = 0;

        // Find key timeframe containing the timestamp
        for (sizeT i = 1; i < _keys.size(); i++)
        {
            if (_time <= _keys[i].timeStamp)
            {
                i1 = i;
                break;
            }
        }

        i0 = i1 > 0 ? i1 - 1 : 0;

        if (_time >= _keys[i0].timeStamp && _time <= _keys[i1].timeStamp)
        {
            float deltaTime = _keys[i1].timeStamp - _keys[i0].timeStamp;
            float k = (_time - _keys[i0].timeStamp) / deltaTime;

            return glm::slerp(_keys[i0].value, _keys[i1].value, k);
        }

        return _keys.back().value;
    }
}










