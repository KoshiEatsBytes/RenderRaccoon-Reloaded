
#include "FreeRoam.h"

#include "Components/FreeCameraComponent.h"

FreeRoam::FreeRoam() : Scene("Free Roam") {}

FreeRoam::~FreeRoam()
= default;

bool FreeRoam::Init()
{
    SetCursorEnabled(false);

    m_cam = CreateObject("FlyCam");
    m_cam->AddComponent<RR::FreeCameraComponent>();
    m_cam->SetPosition(vec3(0,5,10));
    SetMainCamera(m_cam);

    return true;
}

void FreeRoam::PreUpdate(float _deltaTime)
{
}

void FreeRoam::Update(float _deltaTime)
{
}

void FreeRoam::LateUpdate(float _deltaTime)
{
}

void FreeRoam::Destroy()
{
}
