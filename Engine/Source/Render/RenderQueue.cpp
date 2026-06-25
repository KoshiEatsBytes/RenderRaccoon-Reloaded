
#include "RenderQueue.h"

#include "Material.h"
#include "Graphics/GraphicsAPI.h"
#include "Graphics/ShaderProgram.h"


namespace RR
{
    // PRIVATE ---------------------------------------------------------------------------------------------------------

    RenderQueue::RenderQueue()
    = default;

    RenderQueue::~RenderQueue()
    = default;

    // PUBLIC ----------------------------------------------------------------------------------------------------------

    /**
     * @brief Enqueues draw commands
     * @param _command draw command
     */
    void RenderQueue::Submit(const RenderCommand &_command)
    {
        m_commands.push_back(_command);
    }

    /**
     * @brief Will execute all queued draw calls
     * @param _graphicsAPI engine ref to the graphics api
     * @param _camData
     */
    void RenderQueue::Draw(GraphicsAPI& _graphicsAPI, const CameraData& _camData,
        const std::vector<LightData>& _lights)
    {
        // comparison tool for material, dont bind same material twice
        Material* lastMaterial = nullptr;

        for (auto& command : m_commands)
        {
            ShaderProgram* shaderProgram = command.material->GetShaderProgram();

            if (command.material != lastMaterial)
            {
                _graphicsAPI.BindMaterial(command.material);

                // Per material optimizations
                _graphicsAPI.SetDepthTest      (command.material->GetDepthTest());
                _graphicsAPI.SetBackfaceCulling(command.material->GetBackfaceCull());

                // Update camera if material changes
                shaderProgram->SetUniform("uView",      _camData.viewMatrix);
                shaderProgram->SetUniform("uProj",      _camData.projMatrix);
                shaderProgram->SetUniform("uCameraPos", _camData.position);

                // Render lights, 1 for now
                if (!_lights.empty())
                {
                    auto& light = _lights[0];
                    shaderProgram->SetUniform("uLight.color",     light.color);
                    shaderProgram->SetUniform("uLight.direction", glm::normalize(-light.position));
                }

                lastMaterial = command.material;
            }
            // Per call sets
            shaderProgram->SetUniform("uModel", command.modelMatrix);
            shaderProgram->SetUniform("uColor", command.color);

            // Bind and draw meshes
            _graphicsAPI.BindMesh(command.mesh);
            _graphicsAPI.DrawMesh(command.mesh);
        }

        // Unbinds last mesh
        _graphicsAPI.UnbindVertexArray();
        _graphicsAPI.SetBackfaceCulling(false);
        _graphicsAPI.SetDepthTest(true);
        
        m_commands.clear();
    }
}