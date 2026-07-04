
#pragma once

#include "Base/VoxelScene.h"
#include "Components/FreeCameraComponent.h"
#include "WorldGen/WorldGenConfig.h"

class FreeRoamScene : public VoxelScene
{
public:
    explicit FreeRoamScene(const RR::RunInfo& _runInfo, const WORLDGEN::WorldGenConfig& _config);
    explicit FreeRoamScene(const RR::RunInfo& _runInfo);
    ~FreeRoamScene() override;
    
    void Regenerate();
    
protected:
    void OnInit()                    override;
    void PreUpdate(float _deltaTime) override;
    void OnUpdate(float _deltaTime)  override;
    void OnGui()                     override;
    bool InUiMode() const            override;
    
private:
    // Editable world gen config
    WORLDGEN::WorldGenConfig m_draftConfig;

    bool m_diagnostics = false;
    bool m_f3WasDown   = false;
    bool m_uiMode      = false;
    bool m_tabWasDown  = false;
    bool m_fancyLeaves = true;
};