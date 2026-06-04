
#pragma once
#include <RR.h>

class Benchmark : public RR::Scene
{
public:
    Benchmark();
    ~Benchmark() override = default;

protected:
    bool Init()                       override;
    void PreUpdate(float _deltaTime)  override;
    void Update(float _deltaTime)     override;
    void LateUpdate(float _deltaTime) override;
    void Destroy()                    override;

private:
    void SpawnBox(const vec3& _pos, const quat& _rot);

    std::shared_ptr<RR::Material> m_material;
    std::shared_ptr<RR::Mesh>     m_boxMesh;

    float m_spawnTimer = 0.0f;
    float m_fpsTimer   = 0.0f;
    int   m_frameCount = 0;
    int   m_boxCount   = 0;
    static constexpr int kMaxBoxes = 1000;
};