
#pragma once
#include "RR.h"

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

protected:
    bool Init() override;
    void PreUpdate(float _deltaTime) override;
    void Update(float _deltaTime) override;
    void LateUpdate(float _deltaTime) override;
    void Destroy() override;
    void OnGui() override;

private:
    RR::GameObject* m_cam = nullptr;

    std::shared_ptr<RR::Material> m_voxelMat;
    std::unique_ptr<RR::ChunkManager> m_chunkManager;
};

