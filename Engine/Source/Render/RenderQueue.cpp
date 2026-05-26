
#include "RenderQueue.h"
#include "Mesh.h"
#include "Material.h"
#include "Graphics/GraphicsAPI.h"


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
            graphicsAPI.BindMaterial(command.material);
            graphicsAPI.BindMesh(command.mesh);
            graphicsAPI.DrawMesh(command.mesh);
        }

        m_commands.clear();
    }
}