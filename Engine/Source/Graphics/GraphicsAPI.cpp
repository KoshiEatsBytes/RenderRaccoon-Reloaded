
#include "Engine.h"
#include "GraphicsAPI.h"
#include "ShaderProgram.h"
#include "GLFW/glfw3.h"
#include "Helpers/Printer.hpp"
#include "Render/Material.h"
#include "Render/Mesh.h"

namespace RR
{
    // PRIVATE ---------------------------------------------------------------------------------------------------------

    GraphicsAPI::GraphicsAPI() = default;

    GraphicsAPI::~GraphicsAPI() = default;

    // PUBLIC -----------------------------------------------------------------------------------------------------------

    bool GraphicsAPI::Init()
    {
        // Reversed Z needs glClipControl (4.5 feature) detect if present, most modern
        // hardware should support it, but implement fail safe for dated devices
        m_reversedZSupported = GLEW_ARB_clip_control != 0 && glClipControl != nullptr;
        Log("[GRAPHICS] Reversed-Z: ", m_reversedZSupported ? "available (float-depth path)"
                                                            : "unavailable (standard-depth fallback)");

        SetReversedZEnabled(true);

        // Turn off vertical sync
        glfwSwapInterval(0);

        glEnable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_TRUE);

        // culling start disabled
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        return true;
    }

    /**
     * @brief Compiles, links and creates a new shader program
     * @param _vertexShaderSource Vertex shader source code as string
     * @param _fragmentShaderSource Fragment shader source code as string
     * @return new compiled ShaderProgram if successfully, otherwise nullptr
     */
    std::shared_ptr<ShaderProgram> GraphicsAPI::CreateShaderProgram(const std::string& _vertexShaderSource,
                                                                    const std::string& _fragmentShaderSource)
    {
        // VERTEX SHADER -----------------------------------------------------------------------------------------------

        // Load vertex shader from file and compile it
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

        // check if valid path
        if (_vertexShaderSource.empty())
        {
            Error("[VERTEX SHADER] INVALID vertex shader source provided to shader program");
            glDeleteShader(vertexShader);
            return nullptr;
        }

        const char* vertexShaderSourceCStr = _vertexShaderSource.c_str();
        glShaderSource(vertexShader, 1, &vertexShaderSourceCStr, nullptr);
        glCompileShader(vertexShader);

        // Check if vertex has compiled correctly
        GLint success;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
            Error("[VERTEX SHADER] Compilation Failed!");
            InfoLog(infoLog);

            // Clean-up
            glDeleteShader(vertexShader);
            return nullptr;
        }

        // FRAGMENT SHADER ---------------------------------------------------------------------------------------------

        // Load fragment shader from file and compile it
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

        // check if valid path
        if (_fragmentShaderSource.empty())
        {
            Error("[FRAGMENT SHADER] INVALID fragment shader source provided to shader program");

            // clean-up
            glDeleteShader(fragmentShader);
            glDeleteShader(vertexShader);
            return nullptr;
        }

        const char* fragmentShaderSourceCStr = _fragmentShaderSource.c_str();
        glShaderSource(fragmentShader, 1, &fragmentShaderSourceCStr, nullptr);
        glCompileShader(fragmentShader);

        // Check if fragment has compiled correctly
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
            Error("[FRAGMENT SHADER] Compilation Failed!");
            InfoLog(infoLog);

            // clean-up
            glDeleteShader(fragmentShader);
            glDeleteShader(vertexShader);
            return nullptr;
        }

        // SHADER PROGRAM ----------------------------------------------------------------------------------------------

        // Link previous shaders to shader program
        GLuint shaderProgramID = glCreateProgram();
        glAttachShader(shaderProgramID, vertexShader);
        glAttachShader(shaderProgramID, fragmentShader);
        glLinkProgram(shaderProgramID);

        // check if link successful
        glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetProgramInfoLog(shaderProgramID, 512, nullptr, infoLog);
            Error("[SHADER LINKING] Shader Program Linking Failed! Program ID: ", shaderProgramID);
            InfoLog(infoLog);

            // cleanup
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            glDeleteProgram(shaderProgramID);
            return nullptr;
        }

        // Delete shader after gpu upload
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        // after success wrap in shaderProgramID
        return std::make_shared<ShaderProgram>(shaderProgramID);
    }

    const std::shared_ptr<ShaderProgram>& GraphicsAPI::GetDefaultShaderProgram()
    {
        // Init shader program if it has not been already initialized
        if (!m_defaultShaderProgram)
        {
            auto& fileSys = Engine::GetInstance().GetFileSystem();
            auto vertexSrc = fileSys.LoadAssetFileText("Shaders/Default/Default.vert");
            auto fragmentSrc = fileSys.LoadAssetFileText("Shaders/Default/Default.frag");

            m_defaultShaderProgram = CreateShaderProgram(vertexSrc, fragmentSrc);
        }

        return m_defaultShaderProgram;
    }

    /**
     * @brief Uploads vertex data to the GPU via buffer
     * @param _vertices vertices to buffer
     * @return GLuint vertex buffer object
     */
    GLuint GraphicsAPI::CreateVertexBufferObject(const std::vector<float>& _vertices)
    {
        GLuint VBO = 0;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        // transfer vertex data from sys memory to gpu memory
        glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(float), _vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        return VBO;
    }

    /**
     * @brief Uploads element data to the GPU via buffer
     * @param _indices indexes to buffer
     * @return GLuint element buffer object
     */
    GLuint GraphicsAPI::CreateElementBufferObject(const std::vector<uint32_t>& _indices)
    {
        GLuint EBO = 0;
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);

        // transfer element data from sys memory to gpu memory
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices.size() * sizeof(uint32_t), _indices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        return EBO;
    }

    void GraphicsAPI::BindShaderProgram(ShaderProgram* _shaderProgram)
    {
        if (!_shaderProgram)
        {
            Warn("[BINDING - SHADER PROGRAM] Tried to bind INVALID shader program");
            return;
        }
        
        _shaderProgram->Bind();
    }

    void GraphicsAPI::BindMaterial(Material* _material)
    {
        if (!_material)
        {
            Warn("[BINDING - MATERIAL] Tried to bind INVALID material");
            return;
        }

        _material->Bind();
    }

    void GraphicsAPI::BindMesh(Mesh *_mesh)
    {
        if (!_mesh)
        {
            Warn("[BINDING - MESH] Tried to bind INVALID mesh");
            return;
        }

        _mesh->Bind();
    }

    void GraphicsAPI::UnBindMesh(Mesh *_mesh)
    {
        if (!_mesh)
        {
            Warn("[BINDING - MESH] Tried to unbind INVALID mesh");
            return;
        }

        _mesh->UnBind();
    }

    void GraphicsAPI::UnbindVertexArray()
    {
        glBindVertexArray(0);
    }

    void GraphicsAPI::DrawMesh(Mesh *_mesh)
    {
        if (!_mesh)
        {
            Warn("[DRAWING - MESH] Tried to draw INVALID mesh");
            return;
        }

        _mesh->Draw();
    }

    void GraphicsAPI::ClearBuffers()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GraphicsAPI::BeginSceneTarget(int _width, int _height)
    {
        // init fb with viewport
        EnsureRenderTarget(_width, _height);
        glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFBO);
        glViewport(0, 0, _width, _height);
    }

    void GraphicsAPI::BlitSceneToDefault(int _width, int _height)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_sceneFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, _width, _height, 0, 0, _width, _height,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // default frame buffer bound for UI
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GraphicsAPI::SetBackfaceCulling(bool _enabled)
    {
        if (_enabled)
        {
            glEnable(GL_CULL_FACE);
        }
        else
        {
            glDisable(GL_CULL_FACE);
        }
    }

    void GraphicsAPI::SetDepthTest(bool _enabled)
    {
        if (_enabled)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }
    }

    void GraphicsAPI::SetBlend(bool _enabled)
    {
        if (_enabled)
        {
            glEnable(GL_BLEND);
        }
        else
        {
            glDisable(GL_BLEND);
        }
    }

    void GraphicsAPI::SetDepthWrite(bool _enabled)
    {
        glDepthMask(_enabled ? GL_TRUE : GL_FALSE);
    }

    void GraphicsAPI::SetClearColor(const vec4& _color)
    {
        glClearColor(_color.x, _color.y, _color.z, _color.w);
    }

    void GraphicsAPI::SetReversedZEnabled(bool _enabled)
    {
        m_reversedZEnabled = _enabled;
        if (!m_reversedZSupported) return;

        if (IsReversedZ())
        {
            glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
            glClearDepth(0.0);
            glDepthFunc(GL_GREATER);
        }
        else
        {
            glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
            glClearDepth(1.0);
            glDepthFunc(GL_LESS);
        }
    }

    bool GraphicsAPI::IsReversedZ() const
    {
        return m_reversedZSupported && m_reversedZEnabled;
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

    void GraphicsAPI::EnsureRenderTarget(int _width, int _height)
    {
        // Already sized correctly, discard
        if (m_sceneFBO != 0 && _width == m_rtWidth && _height == m_rtHeight) return;

        // Init frame and render buffers
        if (m_sceneFBO == 0)
        {
            glGenFramebuffers (1, &m_sceneFBO);
        }
        if (m_sceneColorRBO == 0)
        {
            glGenRenderbuffers(1, &m_sceneColorRBO);
        }
        if (m_sceneDepthRBO == 0)
        {
            glGenRenderbuffers(1, &m_sceneDepthRBO);
        }

        // Bind FBOs and RBOs
        glBindRenderbuffer(GL_RENDERBUFFER, m_sceneColorRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, _width, _height);

        glBindRenderbuffer(GL_RENDERBUFFER, m_sceneDepthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, _width, _height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFBO);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_RENDERBUFFER, m_sceneColorRBO);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            GL_RENDERBUFFER, m_sceneDepthRBO);

        // check if the frame target completed
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            Error("[GRAPHICS] Scene render target incomplete");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_rtWidth  = _width;
        m_rtHeight = _height;
    }
}

























