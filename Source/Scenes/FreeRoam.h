
#pragma once
#include "RR.h"
#include "Components/FreeCameraComponent.h"
#include "WorldGen/WorldGenConfig.h"

namespace RR
{
    struct Chunk;
    class ChunkManager;
}

class FreeRoam : public RR::Scene
{
public:
    explicit FreeRoam();
    ~FreeRoam() override;

    void Regenerate();

protected:
    bool Init() override;
    void PreUpdate(float _deltaTime) override;
    void Update(float _deltaTime) override;
    void LateUpdate(float _deltaTime) override;
    void Destroy() override;
    void OnGui() override;

private:
    RR::GameObject* m_cam = nullptr;
    RR::FreeCameraComponent* m_camComp = nullptr;

    std::shared_ptr<RR::Material> m_voxelMat;
    std::shared_ptr<RR::Material> m_vegMat;
    std::unique_ptr<RR::ChunkManager> m_chunkManager;
    WORLDGEN::WorldGenConfig m_genConfig;
    WORLDGEN::WorldGenConfig m_draftConfig;

    bool m_uiMode     = false;
    bool m_tabWasDown = false;

};

