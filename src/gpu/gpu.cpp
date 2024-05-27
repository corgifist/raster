#include "gpu/gpu.h"

#define GLAD_GLES2_IMPLEMENTATION
#include "gles2.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <GLFW/glfw3.h>

#include "../ImGui/imgui.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "ImGui/imgui_impl_glfw.h"

namespace Raster {

    GPUInfo GPU::info{};

    static void GLAPIENTRY
    MessageCallback( GLenum source,
                    GLenum type,
                    GLuint id,
                    GLenum severity,
                    GLsizei length,
                    const GLchar* message,
                    const void* userParam )
    {
    fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
                type, severity, message );
    }

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

        glEnable              ( GL_DEBUG_OUTPUT );
        glDebugMessageCallback( MessageCallback, 0 );

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

    Texture GPU::GenerateTexture(uint32_t width, uint32_t height, TexturePrecision precision) {
        GLuint textureHandle;
        glGenTextures(1, &textureHandle);
        glBindTexture(GL_TEXTURE_2D, textureHandle);
        
        auto format = GL_RGBA8;
        if (precision == TexturePrecision::Full) format = GL_RGBA32F;
        if (precision == TexturePrecision::Half) format = GL_RGBA16F;
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Texture texture;
        texture.width = width;
        texture.height = height;
        texture.precision = precision;
        texture.handle = (void*) textureHandle;
        return texture;
    }

    void GPU::UpdateTexture(Texture texture, uint32_t x, uint32_t y, uint32_t w, uint32_t h, void* pixels) {
        glBindTexture(GL_TEXTURE_2D, (uint32_t) uint64_t(texture.handle));

        auto format = GL_UNSIGNED_BYTE;
        if (texture.precision == TexturePrecision::Full) format = GL_FLOAT;
        if (texture.precision == TexturePrecision::Half) format = GL_HALF_FLOAT;
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA, format, pixels);
    }

    Texture GPU::ImportTexture(const char* path) {
        int width, height;
        stbi_uc* image = stbi_load(path, &width, &height, nullptr, 4);

        auto texture = GenerateTexture(width, height);
        UpdateTexture(texture, 0, 0, width, height, image);

        stbi_image_free(image);

        return texture;
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