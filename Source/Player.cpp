
#include "Player.h"

#include "GLFW/glfw3.h"

Player::Player()
{
}

Player::~Player()
{
}

void Player::Init()
{
    AddComponent(new RR::CameraComponent());
    SetPosition(vec3(0.0f, 0.0f, 2.0f));
    AddComponent(new RR::PlayerControllerComponent);

    auto gun = LoadGLTF("Models/Gun/scene.gltf");
    gun->SetParent(this);
    gun->SetPosition(vec3(0.75f, -0.5f, -0.75f));
    gun->SetScale(vec3(-1.0f, 1.0f, 1.0f));

    if (auto anim = gun->FindComponentByType<RR::AnimationComponent>())
    {
        if (auto bullet = gun->FindObjectByName("bullet_33", true))
        {
            bullet->SetActive(false);
        }

        if (auto flash = gun->FindObjectByName("BOOM_35", true))
        {
            flash->SetActive(false);
        }

        anim->Play("shoot", false);
    }
    m_animationComp = gun->FindComponentByType<RR::AnimationComponent>();
}

void Player::Update(float _deltaTime)
{
    GameObject::Update(_deltaTime);

    auto& input = RR::Engine::GetInstance().GetInputManager();

    if (input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
    {
        if (m_animationComp && !m_animationComp->IsPlaying())
        {
            m_animationComp->Play("shoot", false);
        }
    }
}
