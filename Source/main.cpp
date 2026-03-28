#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <ostream>

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

    glfwMakeContextCurrent(window);

    // Init Library to use glew funcs
    if (glewInit() != GLEW_OK)
    {
        std::cout << "Failed to initialize GL library" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Runs until window is closed
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Swap the two buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Out of loop, therefore initialize termination of program
    glfwTerminate();

    return 0;
}
