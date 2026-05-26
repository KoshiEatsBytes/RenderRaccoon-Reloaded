
#pragma once
#include <string>
#include <vector>
#include <memory>

namespace RR
{
    class GameObject
    {
    public:
        friend class Scene;
        virtual ~GameObject();

        virtual void Update(float _deltaTime);

        void MarkForDestroy();

        // getters/setters
        const std::string& GetName() const;
        void SetName(const std::string& _name);
        GameObject* GetParent();
        bool IsAlive() const;

    protected:
        GameObject();

    private:
        bool m_isAlive = true;
        std::string m_name;
        GameObject* m_parent = nullptr;
        std::vector<std::unique_ptr<GameObject>> m_children;
    };
}

