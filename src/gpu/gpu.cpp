#include "gpu/gpu.h"

#define GLAD_GLES2_IMPLEMENTATION
#include "gles2.h"

#include <GLFW/glfw3.h>

#include "../ImGui/imgui.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "ImGui/imgui_impl_glfw.h"

namespace Raster {

    GPUInfo GPU::info{};

    void GPU::Initialize() {
        if (!glfwInit()) {
            throw std::runtime_error("cannot initialize glfw!");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);

        GLFWwindow* display = glfwCreateWindow(1280, 720, "Raster", nullptr, nullptr);
        info.display = display;
        if (!info.display) {
            throw std::runtime_error("cannot create raster window!");
        }
        glfwMakeContextCurrent(display);
        glfwSwapInterval(1);

        if (!gladLoadGLES2((GLADloadfunc) glfwGetProcAddress)) {
            throw std::runtime_error("cannot initialize opengl es pointers!");
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init();
        ImGui_ImplGlfw_InitForOpenGL(display, true);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        glfwSetFramebufferSizeCallback(display, [](GLFWwindow* display, int width, int height) {
            glViewport(0, 0, width, height);
        });

        info.version = std::string((const char*) glGetString(GL_VERSION));
        info.renderer = std::string((const char*) glGetString(GL_RENDERER)) + std::string(" / GLFW ") + glfwGetVersionString();

        std::cout << info.version << std::endl;
        std::cout << info.renderer << std::endl;
    }

    bool GPU::MustTerminate() {
        return glfwWindowShouldClose((GLFWwindow*) info.display);
    }

    void GPU::BeginFrame() {
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void GPU::EndFrame() {
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers((GLFWwindow*) info.display);
    }

    void GPU::Terminate() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow((GLFWwindow*) info.display);
        glfwTerminate();
    }

    void* GPU::GetImGuiContext() {
        return ImGui::GetCurrentContext();
    }
}