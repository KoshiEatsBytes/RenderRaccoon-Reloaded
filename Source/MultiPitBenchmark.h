#pragma once
#include <RR.h>
#include <vector>

class MultiPitBenchmark : public RR::Scene
{
public:
    MultiPitBenchmark();
    ~MultiPitBenchmark() override = default;

protected:
    bool Init() override;
    void PreUpdate(float) override;
    void Update(float _deltaTime) override;
    void LateUpdate(float) override;
    void Destroy() override;

private:
    void BuildPit(const vec3& _center, float _half);
    void SpawnBox(const vec3& _pos, const quat& _rot);

    std::shared_ptr<RR::Material> m_material;
    std::shared_ptr<RR::Mesh>     m_boxMesh;
    std::vector<vec3>             m_pitCenters;
    float                         m_pitHalf = 12.0f;

    float m_spawnTimer = 0.0f;
    float m_fpsTimer   = 0.0f;
    int   m_frameCount = 0;
    int   m_boxCount   = 0;

    // === THE KNOB: rebuild with 1, 2, 3, 4… and compare ===
    static constexpr int   kPitsPerSide = 4;     // 1 = single pile (control), 2 = 4 pits, 3 = 9, 4 = 16
    static constexpr int   kTotalBoxes  = 1000;  // held constant across all runs
    static constexpr float kWallHeight  = 8.0f;
};