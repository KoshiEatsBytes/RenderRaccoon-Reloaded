
#include "Player.h"

#include "GLFW/glfw3.h"
#include "Scene/Components/AudioListenerComponent.h"
#include "Scene/Components/AudioSourceComponent.h"

Player::Player()
{
}

Player::~Player()
{
}

void Player::Init()
{
    SetPosition(vec3(0.0f, 8.0f, 0.0f));

    AddComponent<RR::CameraComponent>();
    m_PCC = AddComponent<RR::PlayerControllerComponent>(1.2f, 0.4f);
    AddComponent<RR::AudioListenerComponent>();


    m_audioComp = AddComponent<RR::AudioSourceComponent>();
    m_shootSfx = m_audioComp->BindTrack("shoot", 1);
    m_jumpSfx  = m_audioComp->BindTrack("jump", 1);
    m_walkSfx  = m_audioComp->BindTrack("step", 1);
    m_jumpSfx->SetSpatialization(false);

    auto gun = LoadGLTF("Models/Gun/scene.gltf");
    gun->SetParent(this);
    gun->SetPosition(vec3(0.75f, -0.5f, -0.75f));
    gun->SetScale(vec3(-1.0f, 1.0f, 1.0f));

    if (auto anim = gun->FindComponentByType<RR::AnimationComponent>())
    {
        if (auto bullet = gun->FindObjectByName("bullet_33", true))
        {
            bullet->SetEnabled(false);
        }

        if (auto flash = gun->FindObjectByName("BOOM_35", true))
        {
            flash->SetEnabled(false);
        }

        anim->Play("shoot", false);
    }
    m_animationComp = gun->FindComponentByType<RR::AnimationComponent>();
}

void Player::PreUpdate(float _deltaTime)
{
    GameObject::PreUpdate(_deltaTime);

    auto& input = RR::Engine::GetInstance().GetInputManager();

    m_shootCooldown -= _deltaTime;
    if (input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT) && m_shootCooldown <= 0.0f)
    {
        if (m_animationComp && !m_animationComp->IsPlaying())
        {
            m_animationComp->Play("shoot", false);

            m_shootSfx.PlayOneShot();
            m_shootCooldown = 0.12f;
        }
    }

    if (m_shootSfx && input.IsKeyPressed(GLFW_KEY_G)) m_shootSfx->SetPitch(0.6f);
    if (m_shootSfx && input.IsKeyPressed(GLFW_KEY_H)) m_shootSfx->SetPitch(1.5f);


    if (input.IsKeyPressed(GLFW_KEY_SPACE) && m_PCC->IsMidJump())
    {
        if (m_jumpSfx)
        {
            if (!m_jumpSfx->IsPlaying())
            {
                m_jumpSfx.PlayOneShot();
            }
        }
    }


    bool walking =
        input.IsKeyPressed(GLFW_KEY_A) ||
        input.IsKeyPressed(GLFW_KEY_S) ||
        input.IsKeyPressed(GLFW_KEY_D) ||
        input.IsKeyPressed(GLFW_KEY_W);

    if (walking && m_PCC && m_PCC->IsOnGround())
    {
        if (m_walkSfx)
        {
            if (!m_walkSfx->IsPlaying())
            {
                m_walkSfx->Play(true);
            }
        }
    }
    else
    {
        if (m_walkSfx && m_walkSfx->IsPlaying())
        {
            m_walkSfx->Stop();
        }
    }
}

void Player::Update(float _deltaTime)
{
    GameObject::Update(_deltaTime);
}
