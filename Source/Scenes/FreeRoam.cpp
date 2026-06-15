#include <cstring>

#include "FreeRoam.h"

#include "Components/FreeCameraComponent.h"
#include "Render/MeshData.h"
#include "Render/Voxels/ChunkMesher.h"
#include "Voxels/Chunk.h"
#include "Voxels/ChunkData.h"
#include "Render/Mesh.h"
#include "Render/RenderQueue.h"
#include "Engine.h"
#include "GLFW/glfw3.h"

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

static void ProveFaceRotation()
{
    int hist[4] = {0,0,0,0};
    for (int z = 0; z < 64; ++z)
        for (int x = 0; x < 64; ++x)
            hist[RR::CHUNK::FaceRotation(x, z)]++;

    const bool deterministic = RR::CHUNK::FaceRotation(123, -456) == RR::CHUNK::FaceRotation(123, -456);
    RR::InfoLog("[ROT] /4096 -> 0:", hist[0], " 1:", hist[1], " 2:", hist[2],
                " 3:", hist[3], " det=", deterministic);
}

static std::shared_ptr<RR::Mesh> BuildTestQuad()
{
    // 2x2 quad at z = -3, facing +Z (toward the spawn camera, which looks -Z), layer = GRASS_TOP.
    const float L = static_cast<float>(RR::CHUNK::BlockTex::GRASS_TOP);   // slice 0

    std::vector<float> verts = {
        // pos                 normal         uv        layer
        -1.f,-1.f,-3.f,   0.f, 0.f, 1.f,   0.f, 0.f,   L,   // bottom-left
         1.f,-1.f,-3.f,   0.f, 0.f, 1.f,   1.f, 0.f,   L,   // bottom-right
         1.f, 1.f,-3.f,   0.f, 0.f, 1.f,   1.f, 1.f,   L,   // top-right
        -1.f, 1.f,-3.f,   0.f, 0.f, 1.f,   0.f, 1.f,   L,   // top-left
    };
    std::vector<std::uint32_t> indices = { 0,1,2, 0,2,3 };   // CCW from the front

    return std::make_shared<RR::Mesh>(RR::VoxelVertexLayout(), verts, indices);
}

static void FillTestSlab(RR::Chunk& _c)
{
    using namespace RR::CHUNK;
    const BlockId grass = static_cast<BlockId>(Block::GRASS);
    const BlockId dirt  = static_cast<BlockId>(Block::DIRT);
    const BlockId stone = static_cast<BlockId>(Block::STONE);

    for (int z = 0; z < kSizeZ; ++z)
        for (int x = 0; x < kSizeX; ++x)
        {
            _c.Set(x, 0, z, stone);
            _c.Set(x, 1, z, stone);
            _c.Set(x, 2, z, dirt);
            _c.Set(x, 3, z, dirt);
            _c.Set(x, 4, z, grass);
        }
    _c.state = State::GENERATED;
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

    m_voxelMat = RR::Material::Load("Materials/Voxel.json");

    auto arr = m_voxelMat->GetTextureArray("uBlockTex");
    assert(arr && arr->GetLayerCount() == CHUNK::Tex::COUNT);

    m_testChunk = std::make_unique<RR::Chunk>(RR::CHUNK::Coord{0, -3});
    FillTestSlab(*m_testChunk);

    const RR::ChunkBorders air{};
    RR::MeshData data = RR::MeshChunk(*m_testChunk, air);
    RR::InfoLog("[CHUNK MESH] quads=", data.indices.size() / 6, " verts=", data.vertices.size() / 9);

    m_testChunk->mesh  = std::make_unique<RR::Mesh>(data.layout, data.vertices, data.indices);
    m_testChunk->state = RR::CHUNK::State::MESHED;

    ProveFaceRotation();

    return true;
}

void FreeRoam::PreUpdate(float _deltaTime)
{
    auto& input = RR::Engine::GetInstance().GetInputManager();

    if (input.IsKeyPressed(GLFW_KEY_ESCAPE))
        RR::Engine::GetInstance().SetShouldClose(true);
}

void FreeRoam::Update(float _deltaTime)
{
    if (!m_voxelMat || !m_testChunk || !m_testChunk->mesh) return;

    const auto& coord = m_testChunk->coord;

    RR::RenderCommand cmd;
    cmd.material    = m_voxelMat.get();
    cmd.mesh        = m_testChunk->mesh.get();
    cmd.modelMatrix = glm::translate(mat4(1.0f),
    vec3(coord.x * RR::CHUNK::kSizeX, 0.0f, coord.z * RR::CHUNK::kSizeZ));
    cmd.color       = vec3(1.0f);

    RR::Engine::GetInstance().GetRenderQueue().Submit(cmd);
}

void FreeRoam::LateUpdate(float _deltaTime)
{
}

void FreeRoam::Destroy()
{
}
