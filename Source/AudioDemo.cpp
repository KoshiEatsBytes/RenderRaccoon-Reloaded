
#include "AudioDemo.h"

#include <cmath>

#include "GLFW/glfw3.h"
#include "Scene/Components/AudioSourceComponent.h"
#include "Scene/Components/AudioListenerComponent.h"

namespace
{
    enum Channel : uInt { ChGeneral, ChEffects, ChMusic };
}

AudioDemo::AudioDemo() : Scene("AudioDemo") {}
AudioDemo::~AudioDemo() = default;

bool AudioDemo::Init()
{
    auto& audio = RR::Engine::GetInstance().GetAudioManager();

    auto player = CreateObject<Player>("Player");
    SetMainCamera(player);
    player->AddComponent(new RR::AudioListenerComponent());

    auto* light = CreateObject("Light");
    auto* lightComp = new RR::LightComponent();
    lightComp->SetColor(vec3(1.0f));
    light->AddComponent(lightComp);
    light->SetPosition(vec3(0.0f, 5.0f, 0.0f));

    auto material = RR::Material::Load("Materials/Brick.json");

    auto* ground = CreateObject("Ground");
    ground->SetPosition(vec3(0.0f, -3.0f, 0.0f));
    vec3 groundExtents (20.f, 2.f, 20.f);
    auto groundMesh = RR::Mesh::CreateBox(groundExtents);
    ground->AddComponent(new RR::MeshComponent(material, groundMesh));
    ground->AddComponent(new RR::ColliderComponent(std::make_shared<RR::BoxCollider>(groundExtents)));
    ground->AddComponent(new RR::PhysicsComponent(RR::BodyType::STATIC, 0.0f, 0.8f));

    m_emitter = CreateObject("Emitter");
    m_emitter->AddComponent(new RR::MeshComponent(material, RR::Mesh::CreateBox()));
    m_emitterSource = m_emitter->AddComponent(new RR::AudioSourceComponent());

    m_emitterSource->BindTrack("step", ChEffects);
    m_emitterSource->SetMinMaxDistance(2.0f, 25.0f);
    m_emitterSource->SetDopplerFactor(2.0f);
    m_emitterSource->Play("step", true);

    audio.BindTrack("shoot", ChEffects);
    audio.BindTrack("jump",  ChEffects);

    RR::Success("[AUDIO DEMO] 1=shoot  2=jump  SPACE=toggle emitter  [=SFX 25%  ]=SFX 100%  ESC=quit");
    return true;
}

void AudioDemo::PreUpdate(float) {}

void AudioDemo::Update(float _deltaTime)
{
    auto& engine = RR::Engine::GetInstance();
    auto& input  = engine.GetInputManager();
    auto& audio  = engine.GetAudioManager();

    m_time += _deltaTime;
    const float x = std::sin(m_time * 1.5f) * 12.0f;
    m_emitter->SetPosition(vec3(x, 0.0f, -3.0f));

    if (JustPressed(GLFW_KEY_1)) audio.PlayOneShot("shoot");
    if (JustPressed(GLFW_KEY_2)) audio.PlayOneShot("jump");

    if (JustPressed(GLFW_KEY_SPACE))
    {
        if (m_emitterSource->IsPlaying("step")) m_emitterSource->Stop("step", 0.3f);
        else                                    m_emitterSource->Play("step", true);
    }

    if (input.IsKeyPressed(GLFW_KEY_LEFT_BRACKET))  audio.SetChannelVolume(ChEffects, 0.25f);
    if (input.IsKeyPressed(GLFW_KEY_RIGHT_BRACKET)) audio.SetChannelVolume(ChEffects, 1.0f);

    if (input.IsKeyPressed(GLFW_KEY_ESCAPE)) engine.SetShouldClose(true);
}

void AudioDemo::LateUpdate(float) {}
void AudioDemo::Destroy() {}

bool AudioDemo::JustPressed(int _key)
{
    const bool down = RR::Engine::GetInstance().GetInputManager().IsKeyPressed(_key);
    const bool edge = down && !m_prevKeys[_key];
    m_prevKeys[_key] = down;
    return edge;
}
