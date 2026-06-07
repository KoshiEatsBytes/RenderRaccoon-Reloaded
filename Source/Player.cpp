
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
    //m_audioComp->LoadAudio("shoot", "Audio/shoot.wav");
    //m_audioComp->LoadAudio("jump", "Audio/jump.wav");
    //m_audioComp->LoadAudio("step", "Audio/step.wav");

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

    if (input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
    {
        if (m_animationComp && !m_animationComp->IsPlaying())
        {
            m_animationComp->Play("shoot", false);

            if (m_audioComp)
            {
                if (m_audioComp->IsPlaying("shoot"))
                {
                    m_audioComp->Stop("shoot");
                }
                m_audioComp->Play("shoot");
            }
        }
    }



    if (input.IsKeyPressed(GLFW_KEY_SPACE) && m_PCC->IsMidJump())
    {
        if (m_audioComp)
        {
            if (!m_audioComp->IsPlaying("jump") )
            {
                m_audioComp->Play("jump");
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
        if (m_audioComp)
        {
            if (!m_audioComp->IsPlaying("step"))
            {
                m_audioComp->Play("step", true);
            }
        }
    }
    else
    {
        if (m_audioComp && m_audioComp->IsPlaying("step"))
        {
            m_audioComp->Stop("step");
        }
    }
}

void Player::Update(float _deltaTime)
{
    GameObject::Update(_deltaTime);
}
