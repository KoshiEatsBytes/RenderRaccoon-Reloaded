
#include <cassert>

#include "VoxelScene.h"
#include "Voxels/Chunk.h"
#include "Voxels/ChunkManager.h"
#include "WorldGen/WorldGen.hpp"

VoxelScene::VoxelScene(const std::string& _name, const RR::RunInfo& _runInfo,
    const WORLDGEN::WorldGenConfig& _genConfig)
    : Scene(_name), m_runInfo(_runInfo), m_genConfig(_genConfig)
{
    // RunInfo seed takes over
    m_genConfig.seed = _runInfo.seed;
}

// If not provided worlgen info use default
VoxelScene::VoxelScene(const std::string& _name, const RR::RunInfo& _runInfo)
    : Scene(_name), m_runInfo(_runInfo)
{
    m_genConfig.seed = _runInfo.seed;
}

VoxelScene::~VoxelScene()
= default;

bool VoxelScene::Init()
{
    // Save machine details
    m_runInfo.gpuName   = m_appData.gpuName;
    m_runInfo.cpuName   = m_appData.cpuName;
    m_runInfo.coreCount = m_appData.coreCount;

    // Load materials and assert entries are correct size
    m_voxelBlocksMat = RR::Material::Load("Materials/Voxel.json");
    {
        auto arr = m_voxelBlocksMat->GetTextureArray("uBlockTex");
        assert(arr && arr->GetLayerCount() == RR::CHUNK::BLOCKTEX::COUNT);
    }
    m_voxelVegMat = RR::Material::Load("Materials/VoxelVegetation.json");
    {
        auto vegArr = m_voxelVegMat->GetTextureArray("uBlockTex");
        assert(vegArr && vegArr->GetLayerCount() == RR::CHUNK::BLOCKTEX::COUNT);
    }

    // Chunk generator lambda
    RR::ChunkGenerator generator = [this](RR::Chunk& chunk) {
        WORLDGEN::GenerateColumn(chunk, m_genConfig);
    };

    // instantiate chunk manager with generator and mats
    m_chunkManager = std::make_unique<RR::ChunkManager>(
        generator, m_voxelBlocksMat, m_voxelVegMat);

    OnInit();
    return true;
}

void VoxelScene::Update(float _deltaTime)
{
    if (m_chunkManager && GetMainCamera())
    {
        const vec3 camPos = GetMainCamera()->GetWorldPosition();
        m_chunkManager->Update(camPos);
        m_chunkManager->SubmitDraws();
    }

    OnUpdate(_deltaTime);
}

void VoxelScene::Destroy()
{
}