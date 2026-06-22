
#pragma once

#include "Base/VoxelScene.h"
#include "Components/FreeCameraComponent.h"
#include "WorldGen/WorldGenConfig.h"

class FreeRoam : public VoxelScene
{
public:
    explicit FreeRoam(const RR::RunInfo& _runInfo, const WORLDGEN::WorldGenConfig& _config);
    explicit FreeRoam(const RR::RunInfo& _runInfo);
    ~FreeRoam() override;
    
    void Regenerate();
    
protected:
    void OnInit() override;
    void PreUpdate(float _deltaTime) override;
    void OnUpdate(float _deltaTime) override;
    void LateUpdate(float _deltaTime) override;
    void OnGui() override;
    
private:
    RR::GameObject*          m_cam     = nullptr;
    RR::FreeCameraComponent* m_camComp = nullptr;
    
    // Editable world gen config
    WORLDGEN::WorldGenConfig m_draftConfig;
    
    bool m_uiMode      = false;
    bool m_tabWasDown  = false;
    bool m_fancyLeaves = true;
};