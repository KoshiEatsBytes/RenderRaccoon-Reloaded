
#pragma once
#include "../Helpers/Types.h"

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
        virtual sizeT GetTypeID() const       = 0;

        GameObject* GetOwner() const;

    protected:
        // for handling the owner
        GameObject* m_owner = nullptr;

    private:
        static sizeT nextID;

    public:
        // Templates ---------------------------------------------------------------------------------------------------

        /**
         * @brief Assigns a static id to each new type fed
         * @tparam T Type to match to ID
         * @return ID assigned to type
         */
        template<typename T>
        static sizeT StaticTypeID()
        {
            static sizeT typeID = nextID++;
            return typeID;
        }
    };

// Macro for overriding vital component interface easily
#define COMPONENT(ComponentClass) \
    static sizeT TypeID() { return Component::StaticTypeID<ComponentClass>(); } \
    sizeT GetTypeID() const override {return TypeID(); }
}
