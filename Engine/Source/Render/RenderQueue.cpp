
#include "RenderQueue.h"

#include "Mesh.h"
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
        sizeT indexCount = 0;

        // comparison tool for material, dont bind same material twice
        Material* lastMaterial = nullptr;

        for (auto& command : m_commands)
        {
            ShaderProgram* shaderProgram = command.material->GetShaderProgram();

            if (command.material != lastMaterial)
            {
                _graphicsAPI.BindMaterial(command.material);

                // Per material settings
                _graphicsAPI.SetDepthTest      (command.material->GetDepthTest());
                _graphicsAPI.SetBackfaceCulling(command.material->GetBackfaceCull());
                _graphicsAPI.SetBlend          (command.material->GetBlend());
                _graphicsAPI.SetDepthWrite     (command.material->GetDepthWrite());

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

            // Get this count
            indexCount += command.mesh->GetIndexCount();
        }

        // Unbinds last mesh
        _graphicsAPI.UnbindVertexArray();
        _graphicsAPI.SetBackfaceCulling(false);
        _graphicsAPI.SetDepthTest(true);
        _graphicsAPI.SetBlend(false);
        _graphicsAPI.SetDepthWrite(true);


        m_lastFrameDraws = m_commands.size();
        m_lastFrameTris  = indexCount / 3;
        m_commands.clear();
    }

    sizeT RenderQueue::GetLastFrameDraws() const
    {
        return m_lastFrameDraws;
    }

    sizeT RenderQueue::GetLastFrameTris() const
    {
        return m_lastFrameTris;
    }
}
