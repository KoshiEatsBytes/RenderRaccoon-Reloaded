
#pragma once

#include "Helpers/Types.h"

namespace RR
{
    class Audio
    {
    public:
        Audio();
        ~Audio();

        void Play(bool _loop = false) const;
        void Stop() const;

        bool IsPlaying() const;

        void SetVolume(float _volume) const;
        float GetVolume() const;

        void SetPosition(const vec3& _pos) const;
        vec3 GetPosition() const;

        bool IsSpatial() const;
        void SetSpatial(bool _spatial);

        maSound* GetSound() const;
        maDecoder* GetDecoder() const;

        static std::shared_ptr<Audio> Load(const std::string& _path, bool _spatial = true);

    private:
        bool m_spatial = true;
        std::unique_ptr<maSound> m_sound;
        std::unique_ptr<maDecoder> m_decoder;
        std::vector<char> m_buffer;
    };
}
