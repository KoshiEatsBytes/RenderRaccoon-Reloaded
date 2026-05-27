
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
     * @param graphicsAPI engine ref to the graphics api
     */
    void RenderQueue::Draw(GraphicsAPI &graphicsAPI)
    {
        for (auto& command : m_commands)
        {
            // material
            graphicsAPI.BindMaterial(command.material);
            command.material->GetShaderProgram()->SetUniform("uModel", command.modelMatrix);

            // Mesh
            graphicsAPI.BindMesh(command.mesh);
            graphicsAPI.DrawMesh(command.mesh);
        }

        m_commands.clear();
    }
}