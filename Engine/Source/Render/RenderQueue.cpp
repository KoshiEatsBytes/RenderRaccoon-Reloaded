
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
    void RenderQueue::Draw(GraphicsAPI& _graphicsAPI, const CameraData& _camData)
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

            // Mesh
            _graphicsAPI.BindMesh(command.mesh);
            _graphicsAPI.DrawMesh(command.mesh);
        }

        m_commands.clear();
    }
}