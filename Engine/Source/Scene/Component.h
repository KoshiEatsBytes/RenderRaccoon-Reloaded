
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

        virtual void Init();
        virtual void PreUpdate(float _deltaTime);
        virtual void Update(float _deltaTime)       = 0;
        virtual void LateUpdate(float _deltaTime);
        virtual sizeT GetTypeID() const             = 0;

        virtual int GetExecutionOrder() const;
        GameObject* GetOwner() const;

        void SetEnabled(bool _enabled);
        bool IsEnabled() const;

    protected:
        virtual void OnEnable();
        virtual void OnDisable();

        bool m_enabled = true;
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
