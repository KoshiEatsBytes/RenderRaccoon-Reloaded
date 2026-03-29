#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

#include "Helpers/ShaderIO.hpp"

struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;
};

Vec2 offset;

void keyCallBack(GLFWwindow* window, int key, int scanCode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
            case GLFW_KEY_UP:
                std::cout << "up" <<std::endl;
                offset.y += 0.25f;
                break;

            case GLFW_KEY_DOWN:
                std::cout << "down" <<std::endl;
                offset.y -= 0.25f;
                break;

            case GLFW_KEY_RIGHT:
                std::cout << "right" <<std::endl;
                offset.x += 0.25f;
                break;

            case GLFW_KEY_LEFT:
                std::cout << "left" <<std::endl;
                offset.x -= 0.25f;
                break;

            default:
                break;
        }
    }
}

int main()
{
    if (!glfwInit())
    {
        return -1;
    }

    // set openGL version to 3.3.0
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    auto* window = glfwCreateWindow(1280, 720, "RenderRaccoon | Reloaded", nullptr, nullptr);

    if (window == nullptr)
    {
        std::cout << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwSetKeyCallback(window, keyCallBack);

    glfwMakeContextCurrent(window);

    // Init Library to use glew funcs
    if (glewInit() != GLEW_OK)
    {
        std::cout << "Failed to initialize GL library" << std::endl;
        glfwTerminate();
        return -1;
    }

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    std::string vertexSource = Helper::ShaderIO::LoadFromFile("Shaders/basic.vert");
    const char* vertexShaderCStr = vertexSource.c_str();
    glShaderSource(vertexShader, 1, &vertexShaderCStr, nullptr);
    glCompileShader(vertexShader);

    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        //get error and show to user
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex Shader Compilation failed: " << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    std::string fragmentSource = Helper::ShaderIO::LoadFromFile("Shaders/basic.frag");
    const char* fragmentShaderCStr = fragmentSource.c_str();
    glShaderSource(fragmentShader, 1, &fragmentShaderCStr, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment Shader Compilation failed: " << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader Program Linking failed: " << infoLog << std::endl;
    }

    // no longer need individual shader objs
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // triangle with different colored vertices
    std::vector<float> vertices = {
        // Position of vertices in screen space
        0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, 0.0f, 1.0f, 0.5f, 0.0f
    };

    // Order of which triangles are being elaborated
    std::vector<unsigned int> indices = {
        0, 1, 2,
        0, 2, 3
    };

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, false, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    GLint uColorLoc = glGetUniformLocation(shaderProgram, "uColor");
    GLint uOffsetLoc = glGetUniformLocation(shaderProgram, "uOffset");

    // Runs until window is closed
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniform4f(uColorLoc, 0.0f, 1.0f, 0.0f, 1.0f);
        glUniform2f(uOffsetLoc, offset.x, offset.y);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Swap the two buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Out of loop, therefore initialize termination of program
    glfwTerminate();

    return 0;
}
