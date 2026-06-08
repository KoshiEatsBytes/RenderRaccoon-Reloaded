
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
        for (auto& command : m_commands)
        {
            auto shaderProgram = command.material->GetShaderProgram();

            // material
            _graphicsAPI.BindMaterial(command.material);
            shaderProgram->SetUniform("uModel", command.modelMatrix);

            // Camera
            shaderProgram->SetUniform("uView", _camData.viewMatrix);
            shaderProgram->SetUniform("uProj", _camData.projMatrix);
            shaderProgram->SetUniform("uCameraPos", _camData.position);

            // Lights
            // for now only use 1 light
            if (!_lights.empty())
            {
                auto& light = _lights[0];
                shaderProgram->SetUniform("uLight.color", light.color);
                shaderProgram->SetUniform("uLight.position", light.position);
            }

            // Mesh
            _graphicsAPI.BindMesh  (command.mesh);
            _graphicsAPI.DrawMesh  (command.mesh);
            _graphicsAPI.UnBindMesh(command.mesh);
        }

        m_commands.clear();
    }
}