
#pragma once
#include "RR.h"
#include "WorldGen/Biome.hpp"

namespace RR
{
    struct Chunk;
    class ChunkManager;
}

class VoxelScene : public RR::Scene
{
    protected:
    // Only constructable by derived
    VoxelScene(const std::string& _name, const RR::RunInfo& _runInfo,
        const WORLDGEN::WorldGenConfig& _genConfig);
    VoxelScene(const std::string& _name, const RR::RunInfo& _runInfo);
    ~VoxelScene() override;

    // Scene hooks
    bool Init()                       override;
    void Update(float _deltaTime)     override;
    void Destroy()                    override;

    // Hooks for derived scene
    virtual void OnInit()                   = 0;
    virtual void OnUpdate(float _deltaTime) = 0;

    // Run data
    RR::RunInfo m_runInfo;
    WORLDGEN::WorldGenConfig m_genConfig;

    // Voxel scene core
    std::shared_ptr<RR::Material>     m_voxelBlocksMat;
    std::shared_ptr<RR::Material>     m_voxelVegMat;
    std::unique_ptr<RR::ChunkManager> m_chunkManager;
};



