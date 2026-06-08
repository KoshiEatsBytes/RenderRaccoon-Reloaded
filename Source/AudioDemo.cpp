
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

    audio.BindTrack("shoot", ChEffects);   
    audio.BindTrack("jump",  ChMusic);     
    m_shoot = audio.GetStatic("shoot");    

    RR::Success("[AUDIO DEMO] --- 2D manager tests ---");
    RR::Log("  1 = shoot one-shot (mash = OVERLAP)     F (hold) = auto-fire");
    RR::Log("  3/4 = shoot vol 0.2/1.0    5/6 = shoot pitch low/high   -> then press 1: clones INHERIT");
    RR::Log("  M = music on (jump loop)   N = music off   R = reload scene (music must CUT OUT)");
    RR::Log("  T = toggle 3D Doppler emitter    [ / ] = SFX bus 25% / 100%    ESC = quit");
    RR::Log("[PLAYER] LMB (hold) = 3D shoot (overlap)   G/H = shoot pitch low/high");
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

    if (JustPressed(GLFW_KEY_1)) m_shoot.PlayOneShot();

    m_autoFire -= _deltaTime;
    if (input.IsKeyPressed(GLFW_KEY_F) && m_autoFire <= 0.0f)
    {
        m_shoot.PlayOneShot();
        m_autoFire = 0.1f;
    }

    if (m_shoot)
    {
        if (JustPressed(GLFW_KEY_3)) { m_shoot->SetVolume(0.2f); RR::Log("[TEST] shoot volume 0.2 -> press 1"); }
        if (JustPressed(GLFW_KEY_4)) { m_shoot->SetVolume(1.0f); RR::Log("[TEST] shoot volume 1.0 -> press 1"); }
        if (JustPressed(GLFW_KEY_5)) { m_shoot->SetPitch(0.6f);  RR::Log("[TEST] shoot pitch 0.6 -> press 1"); }
        if (JustPressed(GLFW_KEY_6)) { m_shoot->SetPitch(1.6f);  RR::Log("[TEST] shoot pitch 1.6 -> press 1"); }
    }

    if (JustPressed(GLFW_KEY_M)) { audio.PlayManaged("jump", true); RR::Log("[TEST] music ON (jump loop)"); }
    if (JustPressed(GLFW_KEY_N)) { audio.StopManaged("jump");       RR::Log("[TEST] music OFF"); }
    if (JustPressed(GLFW_KEY_R))
    {
        RR::Log("[TEST] reloading scene -> music & all 2D voices must go silent");
        engine.GetAppManager().RequestSceneLoad<AudioDemo>();
    }

    if (JustPressed(GLFW_KEY_T))
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
