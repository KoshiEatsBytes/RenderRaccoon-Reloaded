
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
}