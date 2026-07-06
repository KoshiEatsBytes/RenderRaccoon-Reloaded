
#pragma once
#include <vector>

#include "Helpers/Types.h"
#include "Helpers/Common.h"
#include "MeshArena.h"

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
        vec3 color = vec3(1.0f);   // tint multiplied into the shader's uColor (white = none)

        // when not nullptr draw voxel area
        // ptr points at CM owned buffer that is valid for frame
        const MeshArena* arena = nullptr;
        const std::vector<const MeshArena::ArenaHandler*>* slices = nullptr;
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

        // draw-call count of the last frame
        sizeT GetLastFrameDraws() const;
        sizeT GetLastFrameTris() const;

    private:
        std::vector<RenderCommand> m_commands;
        sizeT m_lastFrameDraws = 0;
        sizeT m_lastFrameTris  = 0;
    };
}
