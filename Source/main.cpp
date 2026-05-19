#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

#include "Helpers/ShaderIO.hpp"
#include "Helpers/Printer.hpp"

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
            case GLFW_KEY_W:
                offset.y += 0.1f;
                break;
            case GLFW_KEY_S:
                offset.y -= 0.1f;
                break;
            case GLFW_KEY_A:
                offset.x -= 0.1f;
                break;
            case GLFW_KEY_D:
                offset.x += 0.1f;
                break;

            default:
                break;
        }
    }
}

int main()
{
    rr::printLogo();

    rr::print("This is a log message.");
    rr::warn("This is a warning message.");
    rr::error("This is an error message");

    // check if openGL inits correctly
    if (!glfwInit())
    {
        return -1;
    }

    // Specifies the version of openGL used
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Inits window
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "RenderRaccoon", nullptr, nullptr);

    // Checks if the window initiated correctly
    if (!window)
    {
        rr::error("Window could not be created, terminating.");
        glfwTerminate();
        return -1;
    }

    // Polls for input events
    glfwSetKeyCallback(window, keyCallBack);

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        rr::error("Could not initialize GLEW library, terminating.");
        glfwTerminate();
        return -1;
    }

    // load shader from file and compile it
    std::string vertexShaderSource = rr::ShaderIO::LoadFromFile("Shaders/basic.vert");
    auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vertexShaderCstr = vertexShaderSource.c_str();
    glShaderSource(vertexShader, 1, &vertexShaderCstr, nullptr);
    glCompileShader(vertexShader);

    // check if shared has compiled correctly
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

    // Get error and show it to the user
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        rr::error("Vertex shader compilation has failed: ", infoLog);
    }

    // load fragment shader and compile it
    std::string fragmentShaderSource = rr::ShaderIO::LoadFromFile("Shaders/basic.frag");
    auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragmentShaderCstr = fragmentShaderSource.c_str();
    glShaderSource(fragmentShader, 1, &fragmentShaderCstr, nullptr);
    glCompileShader(fragmentShader);

    // check error and get it to user
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        rr::error("Fragment shader compilation has failed: ", infoLog);
    }

    // Create the program for the graphics card
    auto shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check if linking succeeded
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        rr::error("Shader program linking failed: ", infoLog);
    }

    // Once linking is done, we no longer need individual shader objects
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Triangle made with vertices
    // Color of each vertices is on the right
    std::vector<float> vertices
    {
        0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.4f, 0.4f, 0.0f, 0.0f, 1.0f, 0.0f,
        -0.4f, 0.4f, 0.0f, 0.0f, 1.0f, 0.0f,
        -0.4f, -0.4f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.4f, -0.4f, 0.0f, 0.0f, 1.0f, 0.0f
    };

    // Instead of re-rendering vertices, use them from the existent array
    std::vector<unsigned int> indices =
    {
        0, 1, 4,   // top border
        1, 5, 4,
        1, 2, 5,   // left border
        6, 5, 2,
        2, 3, 6,   // bottom border
        3, 7, 6,
        3, 0, 7,   // right border
        0, 4, 7,   // ← fixed
        4, 5, 6,   // fill
        4, 6, 7
    };

    // Upload triangle data to the gpu, via buffer
    GLuint vbo; // Vertex buffer object
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // transfer vertex data from sys memory to gpu memory
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    // buffer from sys memory can be unbound
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // tell shader program how to interpret this shader, using a vao
    GLuint vao; // Vertex array object
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao); //vao becomes active
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    // How the data should be interpreted
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, false, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    // buffer can be unbound
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    //Get uniform and tints it
    auto uColorLoc = glGetUniformLocation(shaderProgram, "uColor");
    auto uOffsetLoc = glGetUniformLocation(shaderProgram, "uOffset");



    // Run as long as window is open
    while (!glfwWindowShouldClose(window))
    {
        // Paints the window 1 color
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // activate shader perogram
        glUseProgram(shaderProgram);
        glUniform4f(uColorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
        glUniform2f(uOffsetLoc, offset.x, offset.y);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Window should close, so terminate openGL
    glfwTerminate();

    return 0;
}

