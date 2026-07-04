
#pragma once
#include "RR.h"
#include "WorldGen/Biome.hpp"

namespace RR
{
    struct Chunk;
    class  ChunkManager;
    class  FreeCameraComponent;
}

class VoxelScene : public RR::Scene
{
protected:
    // Only constructable by derived
    explicit VoxelScene(const RR::RunInfo& _runInfo, const WORLDGEN::WorldGenConfig& _genConfig);
    explicit VoxelScene(const RR::RunInfo& _runInfo);
    ~VoxelScene() override;

    // Scene hooks
    bool Init()                        override;
    void PreUpdate(float _deltaTime)   override;
    void Update(float _deltaTime)      override;
    void LateUpdate(float _deltaTime)  override;
    void Destroy()                     override;
    void OnGui()                       override;

    // Hooks for derived scene
    virtual void OnInit()                   = 0;
    virtual void OnUpdate(float _deltaTime) = 0;

    // Hooks for menu logic
    virtual void OnPausePrimary();
    virtual void OnPauseSecondary();
    virtual void OnPauseEnter();
    virtual void OnPauseExit();
    virtual bool InUiMode() const;

    void ApplyInputMode();

    // Menu info setters
    void SetPrimaryButtonText  (const std::string& _text);
    void SetSecondaryButtonText(const std::string& _text);
    void SetResumable          (bool _resumable);

    // Run data
    RR::RunInfo m_runInfo;
    WORLDGEN::WorldGenConfig m_genConfig;

    // Voxel scene core
    std::shared_ptr<RR::Material>     m_voxelBlocksMat;
    std::shared_ptr<RR::Material>     m_voxelVegMat;
    std::unique_ptr<RR::ChunkManager> m_chunkManager;
    std::unique_ptr<RR::VoxelSkybox>  m_skybox;

    // Camera
    RR::GameObject*          m_cam     = nullptr;
    RR::FreeCameraComponent* m_camComp = nullptr;

    // fog
    float m_fogEnd       = 0.0f;
    float m_fogMoveSpeed = 3.0f;

    // Menu data
    bool m_escWasDown  = false;
    bool m_paused      = false;
    int  m_dimStrenght = 160;

    float buttonWidthFrac  = 0.8f;
    float buttonHeight = 4.5f;
    float titleFontScale = 4.8f;
    float buttonsTitleSpacing = 1.8f;
    float buttonsInnerSpacing = 1.3f;
    float buttonsTextSize = 2.f;

private:
    // Menu logic
    std::string m_pausePrimaryLabel   = "Restart Benchmark";
    std::string m_pauseSecondaryLabel = "Exit to Main Menu";
    bool m_allowResume = false;
};



