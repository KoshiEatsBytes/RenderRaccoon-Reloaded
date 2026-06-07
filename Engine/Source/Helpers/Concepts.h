
#pragma once
#include <type_traits>

namespace RR
{
    // Only allow classes that inherit from GO
    class GameObject;
    template<typename T>
    concept GameObjectType = std::is_base_of_v<GameObject, T>;

    // Only allow classes that inherit from Comp
    class Component;
    template<typename T>
    concept ComponentType = std::is_base_of_v<Component, T>;

    class Scene;
    template<typename T>
    concept SceneType = std::is_base_of_v<Scene, T>;

    class ISubSystem;
    template<typename T>
    concept SubSystemType = std::is_base_of_v<ISubSystem, T>;

    class AudioVoice;
    template<typename T>
    concept AudioType = std::is_base_of_v<AudioVoice, T>;
}