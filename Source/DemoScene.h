
#pragma once
#include <memory>
#include <vector>
#include <string>
#include <utility>

#include <RR.h>

/**
 * @brief Hand-translated from "scene.json": a walled arena with platforms, a stepped
 *        ladder, two stacks of dynamic cubes, a row of dynamic spheres, a player and a light.
 */
class DemoScene : public RR::Scene
{
public:
    DemoScene();
    ~DemoScene() override;

protected:
    bool Init()                       override;
    void PreUpdate(float _deltaTime)  override;
    void Update(float _deltaTime)     override;
    void LateUpdate(float _deltaTime) override;
    void Destroy()                    override;

private:
    // Reuse one box mesh per distinct size instead of rebuilding identical geometry.
    std::shared_ptr<RR::Mesh> BoxMesh(const vec3& _extents);

    RR::GameObject* MakeStaticBox (const std::string& _name, const vec3& _pos,
                                   const vec3& _extents, const vec3& _color = vec3(1.0f));
    RR::GameObject* MakeDynamicBox(const std::string& _name, const vec3& _pos,
                                   const vec3& _extents, float _mass, const vec3& _color);
    RR::GameObject* MakeDynamicSphere(const std::string& _name, const vec3& _pos,
                                      float _radius, float _mass, const vec3& _color);

    std::shared_ptr<RR::Material>                            m_material;   // shared checker material
    std::vector<std::pair<vec3, std::shared_ptr<RR::Mesh>>> m_boxMeshes;  // size -> mesh cache
};
