
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
        glfwSwapInterval(0); // Turn off vertical sync
        glEnable(GL_DEPTH_TEST);
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

    void GraphicsAPI::SetClearColor(const float _r, const float _g, const float _b, const float _a)
    {
        glClearColor(_r, _g, _b, _a);
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

}

























