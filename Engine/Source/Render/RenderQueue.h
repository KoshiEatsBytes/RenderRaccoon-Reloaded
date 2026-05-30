
#pragma once
#include <vector>

#include "Types.h"
#include "Common.h"

namespace RR
{
    class Mesh;
    class Material;
    class GraphicsAPI;

    /**
     * Contains the render data to render a gameObject
     */
    struct RenderCommand
    {
        Mesh* mesh = nullptr;
        Material* material = nullptr;
        mat4 modelMatrix;
    };

    class RenderQueue
    {
        RenderQueue();
        ~RenderQueue();

    public:
        // Only constructable by engine
        friend class Engine;

        // Delete copy
        RenderQueue(const RenderQueue &other) = delete;
        RenderQueue & operator=(const RenderQueue &other) = delete;

        // Delete move
        RenderQueue(RenderQueue &&other) noexcept = delete;
        RenderQueue & operator=(RenderQueue &&other) noexcept = delete;


        void Submit(const RenderCommand& _command);
        void Draw(GraphicsAPI& _graphicsAPI, const CameraData& _camData,
            const std::vector<LightData>& _lights);

    private:
        std::vector<RenderCommand> m_commands;
    };
}
