
#pragma once

namespace RR
{
    class GameObject;

    class Component
    {
    public:
        friend class GameObject;

        Component();
        virtual ~Component();

        virtual void Update(float _deltaTime) = 0;

        GameObject* GetOwner() const;

    protected:
        // for handling the owner
        GameObject* m_owner = nullptr;
    };
}