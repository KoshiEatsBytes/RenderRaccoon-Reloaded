
#pragma once
#include <vector>
#include "Types.h"

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
        Mat4 modelMatrix;
    };

    /**
     * Global projection and view for a scene
     */
    struct CameraData
    {
        Mat4 viewMatrix;
        Mat4 projMatrix;
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
        void Draw(GraphicsAPI& _graphicsAPI, const CameraData& _camData);

    private:
        std::vector<RenderCommand> m_commands;
    };
}
