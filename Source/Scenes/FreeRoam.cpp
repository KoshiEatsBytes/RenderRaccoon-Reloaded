#include <cstring>

#include "FreeRoam.h"

#include "Components/FreeCameraComponent.h"
#include "Render/MeshData.h"
#include "Render/Voxels/ChunkMesher.h"
#include "Voxels/Chunk.h"
#include "Voxels/ChunkData.h"

FreeRoam::FreeRoam() : Scene("Free Roam") {}

FreeRoam::~FreeRoam()
= default;

static std::uint32_t Hash(std::initializer_list<std::uint32_t> vals)
{
    std::uint32_t h = 2166136261u;                       // FNV-ish mix
    for (std::uint32_t v : vals) { h ^= v; h *= 16777619u; h ^= h >> 13; }
    return h;
}

static bool ProveVoxelIndex()
{
    for (int z = 0; z < RR::CHUNK::kSizeZ; ++z)
    {
        for (int x = 0; x < RR::CHUNK::kSizeX; ++x)
        {
            for (int y = 0; y < RR::CHUNK::kSizeY; ++y)
            {
                int i  = RR::CHUNK::Index(x, y, z);
                int dy = i % RR::CHUNK::kSizeY;
                int dx = (i / RR::CHUNK::kSizeY) % RR::CHUNK::kSizeX;
                int dz = (i / RR::CHUNK::kSizeY) / RR::CHUNK::kSizeX;
                if (dx != x || dy != y || dz != z)
                {
                    RR::Error("[VOXEL] index round-trip FAILED at ", x, ",", y, ",", z); return false;
                }
            }
        }
    }

    RR::Success("[VOXEL] VoxelIndex round-trips for all ", RR::CHUNK::kVoxelsPerChunk, " cells.");
    return true;
}

static bool ProveDeterministicFill()
{
    auto fill = [](RR::Chunk& c, std::uint32_t seed)
    {
        for (int z = 0; z < RR::CHUNK::kSizeZ; ++z)
            for (int x = 0; x < RR::CHUNK::kSizeX; ++x)
                for (int y = 0; y < RR::CHUNK::kSizeY; ++y)
                {
                    std::uint32_t h = Hash({ seed,
                                             std::uint32_t(c.coord.x * 16 + x),
                                             std::uint32_t(y),
                                             std::uint32_t(c.coord.z * 16 + z) });
                    c.Set(x, y, z, RR::CHUNK::BlockId(h % std::uint32_t(RR::CHUNK::Block::COUNT)));
                }
        c.state = RR::CHUNK::State::GENERATED;
    };

    auto a = std::make_unique<RR::Chunk>(RR::CHUNK::Coord{3, -7});
    auto b = std::make_unique<RR::Chunk>(RR::CHUNK::Coord{3, -7});
    fill(*a, 12345);
    fill(*b, 12345);

    if (std::memcmp(a->voxels.data(), b->voxels.data(), RR::CHUNK::kVoxelsPerChunk) != 0)
    {
        RR::Error("[VOXEL] determinism FAILED — same seed produced different bytes.");
        return false;
    }

    RR::Success("[VOXEL] same seed+coord => byte-identical chunk.");
    return true;
}

static std::size_t QuadCount(const RR::MeshData& _m)
{
    return _m.indices.size() / 6;
}

static void FillBox(RR::Chunk& _c, int _ox,int _oy,int _oz, int _ax,int _ay,int _az, RR::CHUNK::BlockId _id)
{
    for (int z = 0; z < _az; ++z)
        for (int x = 0; x < _ax; ++x)
            for (int y = 0; y < _ay; ++y)
                _c.Set(_ox + x, _oy + y, _oz + z, _id);
}

static bool RunMesherProofs()
{
    const RR::ChunkBorders air{};                                        // STEP 2: no neighbours -> all air
    const RR::CHUNK::BlockId stone = static_cast<RR::CHUNK::BlockId>(RR::CHUNK::Block::STONE);

    struct Case { int a, b, c; };
    const Case cases[] = { {1,1,1}, {2,1,1}, {3,3,3} };              // expected 6, 10, 54

    bool allOk = true;
    for (const auto& k : cases)
    {
        auto chunk = std::make_unique<RR::Chunk>(RR::CHUNK::Coord{0, 0});
        FillBox(*chunk, 2, 2, 2, k.a, k.b, k.c,  stone);

        const std::size_t got      = QuadCount(MeshChunk(*chunk, air));
        const std::size_t expected = 2 * (k.a * k.b + k.b * k.c + k.c * k.a);

        if (got != expected)
        {
            RR::Error("[MESH] proof FAILED: ", k.a, "x", k.b, "x", k.c, " -> ", got, " quads (expected ", expected, ")");
            allOk = false;
        }
        else
        {
            RR::Success("[MESH] ", k.a, "x", k.b, "x", k.c, " solid box -> ", got, " quads (shell only).");
        }
    }
    return allOk;
}

bool FreeRoam::Init()
{
    SetCursorEnabled(false);

    m_cam = CreateObject("FlyCam");
    m_cam->AddComponent<RR::FreeCameraComponent>();
    SetMainCamera(m_cam);

    ProveVoxelIndex();
    ProveDeterministicFill();

    RunMesherProofs();

    auto voxelMat = RR::Material::Load("Materials/Voxel.json");
    RR::InfoLog("[VOXEL MAT] loaded=", voxelMat != nullptr);

    auto arr = voxelMat->GetTextureArray("uBlockTex");
    assert(arr && arr->GetLayerCount() == CHUNK::Tex::COUNT);

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
