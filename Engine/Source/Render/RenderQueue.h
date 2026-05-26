
#pragma once
#include <vector>

namespace RR
{
    class Mesh;
    class Material;
    class GraphicsAPI;

    struct RenderCommand
    {
        Mesh* mesh = nullptr;
        Material* material = nullptr;
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
        void Draw(GraphicsAPI& graphicsAPI);

    private:
        std::vector<RenderCommand> m_commands;
    };
}
