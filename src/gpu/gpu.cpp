#include "gpu/gpu.h"
#include "raster.h"
#include <filesystem>
#include <mutex>

#define GLAD_GLES2_IMPLEMENTATION
#include "gles2.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "../ImGui/imgui.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "ImGui/imgui_impl_glfw.h"

#include "image/image.h"

#include "nfd/nfd_glfw3.h"
#include "common/synchronized_value.h"

#define HANDLE_TO_GLUINT(x) ((uint32_t) (uint64_t) (x))
#define GLUINT_TO_HANDLE(x) ((void*) (uint64_t) (x))

namespace Raster {


    GPUInfo GPU::info{};
    Shader GPU::s_basicShader;

   void MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param)
    {
        if (type != GL_DEBUG_TYPE_PERFORMANCE && type != GL_DEBUG_TYPE_ERROR) return;
        auto source_str = [source]() -> std::string {
            switch (source)
            {
                case GL_DEBUG_SOURCE_API: return "API";
                case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "WINDOW SYSTEM";
                case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER COMPILER";
                case GL_DEBUG_SOURCE_THIRD_PARTY:  return "THIRD PARTY";
                case GL_DEBUG_SOURCE_APPLICATION: return "APPLICATION";
                case GL_DEBUG_SOURCE_OTHER: return "OTHER";
                default: return "UNKNOWN";
            }
        }();

        auto type_str = [type]() {
            switch (type)
            {
            case GL_DEBUG_TYPE_ERROR: return "ERROR";
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED_BEHAVIOR";
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "UNDEFINED_BEHAVIOR";
            case GL_DEBUG_TYPE_PORTABILITY: return "PORTABILITY";
            case GL_DEBUG_TYPE_PERFORMANCE: return "PERFORMANCE";
            case GL_DEBUG_TYPE_MARKER:  return "MARKER";
            case GL_DEBUG_TYPE_OTHER: return "OTHER";
                default: return "UNKNOWN";
            }
        }();

        auto severity_str = [severity]() {
            switch (severity) {
            case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFICATION";
            case GL_DEBUG_SEVERITY_LOW: return "LOW";
            case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
            case GL_DEBUG_SEVERITY_HIGH: return "HIGH";
                default: return "UNKNOWN";
            }
        }();

        std::cout << source_str       << ", " 
                        << type_str     << ", " 
                        << severity_str << ", " 
                        << id           << ": " 
                        << message      << std::endl;
    }

    static GLenum InterpretTextureWrappingAxis(TextureWrappingAxis t_axis) {
        switch (t_axis) {
            case TextureWrappingAxis::S: return GL_TEXTURE_WRAP_S;
            default: return GL_TEXTURE_WRAP_T;
        }
    }

    static GLint InterpretTextureWrappingMode(TextureWrappingMode t_mode) {
        switch (t_mode) {
            case TextureWrappingMode::Repeat: return GL_REPEAT;
            case TextureWrappingMode::MirroredRepeat: return GL_MIRRORED_REPEAT;
            case TextureWrappingMode::ClampToEdge: return GL_CLAMP_TO_EDGE;
            default: return GL_CLAMP_TO_BORDER;
        }
    }

    static GLenum InterpretTextureFilteringOperation(TextureFilteringOperation t_operation) {
        switch (t_operation) {
            case TextureFilteringOperation::Magnify: return GL_TEXTURE_MAG_FILTER;
            default: return GL_TEXTURE_MIN_FILTER;
        }
    }

    static GLint InterpretTextureFilteringMode(TextureFilteringMode t_mode) {
        switch (t_mode) {
            case TextureFilteringMode::Linear: return GL_LINEAR;
            default: return GL_NEAREST;
        }
    }

    static GLint InterpretTextureInfo(int channels, TexturePrecision precision) {
        auto format = GL_RGBA8;
        switch (channels) {
            case 1: {
                if (precision == TexturePrecision::Usual) format = GL_R8;
                if (precision == TexturePrecision::Half) format = GL_R16F;
                if (precision == TexturePrecision::Full) format = GL_R32F;
                break;
            }
            case 2: {
                if (precision == TexturePrecision::Usual) format = GL_RG8;
                if (precision == TexturePrecision::Half) format = GL_RG16F;
                if (precision == TexturePrecision::Full) format = GL_RG32F;
                break;
            }
            case 3: {
                if (precision == TexturePrecision::Usual) format = GL_RGB8;
                if (precision == TexturePrecision::Half) format = GL_RGB16F;
                if (precision == TexturePrecision::Full) format = GL_RGB32F;
                break;
            }
            case 4: {
                if (precision == TexturePrecision::Usual) format = GL_RGBA8;
                if (precision == TexturePrecision::Half) format = GL_RGBA16F;
                if (precision == TexturePrecision::Full) format = GL_RGBA32F;
                break;
            }
        }
        return format;
    }

    static GLint InterpretTextureChannels(int channels) {
        switch (channels) {
            case 1: return GL_RED;
            case 2: return GL_RG;
            case 3: return GL_RGB;
            case 4: return GL_RGBA;
        }
        return GL_RGB;
    }

    static GLenum InterpretArrayBufferUsage(ArrayBufferUsage usage) {
        if (usage == ArrayBufferUsage::Static) return GL_STATIC_READ;
        return GL_STREAM_COPY;
    }

    static GLenum InterpretArrayBufferType(ArrayBufferType type) {
        if (type == ArrayBufferType::ShaderStorageBuffer) return GL_SHADER_STORAGE_BUFFER;
        return GL_ARRAY_BUFFER;
    }

    static unsigned int RSHash(const std::string& str)
    {
        unsigned int b    = 378551;
        unsigned int a    = 63689;
        unsigned int hash = 0;

        for(std::size_t i = 0; i < str.length(); i++)
        {
            hash = hash * a + str[i];
            a    = a * b;
        }

        return (hash & 0x7FFFFFFF);
    }


    Texture::Texture() {
        this->handle = nullptr;
    }

    Framebuffer::Framebuffer() {
        this->handle = nullptr;
    }

    Sampler::Sampler(uint64_t handle) {
        this->handle = (void*) handle;
    }

    Sampler::Sampler() {
        this->handle = nullptr;
    }

    Shader::Shader() {
        this->handle = nullptr;
    }

    Shader::Shader(ShaderType type, void* handle) {
        this->type = type;
        this->handle = handle;
    }

    static std::thread::id s_mainThreadID;
    static std::unordered_map<void*, std::unordered_map<std::string, int>> shaderRegistry;

    static int s_width, s_height;

    static std::thread s_renderingThread;
    static std::function<void()> s_renderingFunction;
    static bool s_running;

    static SynchronizedValue<std::string> s_targetTitle;
    static std::mutex s_resizeMutex;

    void GPU::Initialize() {
        s_mainThreadID = std::this_thread::get_id();
        if (!glfwInit()) {
            throw std::runtime_error("cannot initialize glfw!");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow* display = glfwCreateWindow(1280, 720, "Raster", nullptr, nullptr);
        s_width = 1280;
        s_height = 720;
        info.display = display;
        if (!info.display) {
            throw std::runtime_error("cannot create raster window!");
        }
    }

    void GPU::InitializeImGui() {
        auto display = (GLFWwindow*) info.display;
        glfwMakeContextCurrent(display);
        glfwSwapInterval(1);

        if (!gladLoadGLES2((GLADloadfunc) glfwGetProcAddress)) {
            throw std::runtime_error("cannot initialize opengl es pointers!");
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui_ImplOpenGL3_Init();
        ImGui_ImplGlfw_InitForOpenGL(display, true);

        SetupContextState();

        glfwSetFramebufferSizeCallback(display, [](GLFWwindow* display, int width, int height) {
            RASTER_SYNCHRONIZED(s_resizeMutex);
            s_width = width;
            s_height = height;
        });

        info.version = std::string((const char*) glGetString(GL_VERSION));
        info.renderer = std::string((const char*) glGetString(GL_RENDERER)) + std::string(" / GLFW ") + glfwGetVersionString();

        GLint maxTextureSize;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
        info.maxTextureSize = maxTextureSize;

        GLint maxViewportDims[2];
        glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxViewportDims);
        info.maxViewportX = maxViewportDims[0];
        info.maxViewportY = maxViewportDims[1];

        GLint maxUniformComponents;
        glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &maxUniformComponents);
        DUMP_VAR(maxUniformComponents);

        DUMP_VAR(info.maxTextureSize);
        DUMP_VAR(info.maxViewportX);
        DUMP_VAR(info.maxViewportY);

        std::cout << info.version << std::endl;
        std::cout << info.renderer << std::endl;

        s_basicShader = GPU::GenerateShader(ShaderType::Vertex, "basic/shader");
    }

    void GPU::SetRenderingFunction(std::function<void()> t_function) {
        s_renderingFunction = t_function;
    }

    void GPU::StartRenderingThread() {
        s_running = true;
        s_renderingThread = std::thread([]() {
            InitializeImGui();
            glfwMakeContextCurrent((GLFWwindow*) info.display);
            static int s_viewportWidth = 0, s_viewportHeight = 0;
            while (s_running) {
                if (s_viewportWidth != s_width || s_viewportHeight != s_height) {
                    RASTER_SYNCHRONIZED(s_resizeMutex);
                    glViewport(0, 0, s_width, s_height);
                    s_viewportWidth = s_width;
                    s_viewportHeight = s_height;
                }
                s_renderingFunction();
                glfwSwapBuffers((GLFWwindow*) info.display);
            }
        });
        while (!GPU::MustTerminate()) {
            glfwPollEvents();
            auto targetTitle = s_targetTitle.Get();
            static std::string s_cachedTitle = "";
            if (targetTitle != s_cachedTitle) {
                glfwSetWindowTitle((GLFWwindow*) info.display, targetTitle.c_str());
                s_cachedTitle = targetTitle;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            s_running = true;
        }
        s_running = false;
        s_renderingThread.join();
    }

    void GPU::Flush() {
        auto fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        glFlush();
        glClientWaitSync(fence, 0, GL_TIMEOUT_IGNORED);
    }

    void* GPU::ReserveContext() {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        auto newContext = glfwCreateWindow(8, 8, "Raster Background Context", nullptr, (GLFWwindow*) info.display);
        if (!newContext) {
            std::cout << "failed to create background context!" << std::endl;
        }
        return newContext;
    }

    void GPU::SetupContextState() {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        glEnable              ( GL_DEBUG_OUTPUT );
        glDebugMessageCallback( MessageCallback, 0 );

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    }

    void GPU::SetCurrentContext(void* context) {
        glfwMakeContextCurrent((GLFWwindow*) context);
        SetupContextState();
    }

    double GPU::GetTime() {
        return glfwGetTime();
    }

    bool GPU::MustTerminate() {
        return glfwWindowShouldClose((GLFWwindow*) info.display);
    }

    void GPU::BeginFrame() {

        GPU::BindFramebuffer(std::nullopt);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void GPU::EndFrame() {
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        auto& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
    }

    Texture GPU::GenerateTexture(uint32_t width, uint32_t height, int channels, TexturePrecision precision, bool mipmapped) {
        width = std::max(1, (int) width);
        height = std::max(1, (int) height);
        GLuint textureHandle;
        glGenTextures(1, &textureHandle);
        glBindTexture(GL_TEXTURE_2D, textureHandle);
        
        auto format = InterpretTextureInfo(channels, precision);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexStorage2D(GL_TEXTURE_2D, mipmapped ? 8 : 1, format, width, height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Texture texture;
        texture.width = width;
        texture.height = height;
        texture.precision = precision;
        texture.channels = channels;
        texture.handle = GLUINT_TO_HANDLE(textureHandle);
        return texture;
    }

    void GPU::GenerateMipmaps(Texture texture) {
        GLuint textureHandle = HANDLE_TO_GLUINT(texture.handle);
        glBindTexture(GL_TEXTURE_2D, textureHandle);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    void GPU::UpdateTexture(Texture texture, uint32_t x, uint32_t y, uint32_t w, uint32_t h, int channels, void* pixels) {
        glBindTexture(GL_TEXTURE_2D, HANDLE_TO_GLUINT(texture.handle));

        auto format = GL_UNSIGNED_BYTE;
        if (texture.precision == TexturePrecision::Full) format = GL_FLOAT;
        if (texture.precision == TexturePrecision::Half) format = GL_HALF_FLOAT;
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, InterpretTextureChannels(channels), format, pixels);
    }

    Texture GPU::ImportTexture(const char* path) {
        auto imageCandidate = ImageLoader::Load(path);

        if (imageCandidate.has_value()) {
            auto& image = imageCandidate.value();
            auto precision = TexturePrecision::Usual;
            if (image.precision == ImagePrecision::Half) {
                precision = TexturePrecision::Half;
            } else if (image.precision == ImagePrecision::Full) {
                precision = TexturePrecision::Full;
            }

            auto texture = GenerateTexture(image.width, image.height, image.channels, precision);

            UpdateTexture(texture, 0, 0, texture.width, texture.height, texture.channels, image.data.data());
            glGenerateMipmap(GL_TEXTURE_2D);
            
            return texture;
        } 

        return Texture();
    }

    void GPU::DestroyTexture(Texture texture) {
        GLuint textureHandle = (uint32_t) (uint64_t) texture.handle;
        glDeleteTextures(1, &textureHandle);
    }

    void GPU::BindTextureToShader(Shader shader, std::string name, Texture texture, int unit) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, HANDLE_TO_GLUINT(texture.handle));
        SetShaderUniform(shader, name, unit);
    }

    // TODO: return std::optional<Framebuffer> instead of Framebuffer
    Framebuffer GPU::GenerateFramebuffer(uint32_t width, uint32_t height, std::vector<Texture> attachments) {
        width = std::max(1, (int) width);
        height = std::max(1, (int) height);
        Framebuffer fbo;
        GLuint fboHandle;
        glGenFramebuffers(1, &fboHandle);

        glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);

        int attachmentIndex = 0;
        std::vector<GLuint> attachmentBuffers;
        for (auto& attachment : attachments) {
            GLuint textureHandle = (GLuint) (uint64_t) attachment.handle;
            glBindTexture(GL_TEXTURE_2D, textureHandle);
            attachmentBuffers.push_back(GL_COLOR_ATTACHMENT0 + attachmentIndex);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachmentIndex++, GL_TEXTURE_2D, textureHandle, 0);
        }

        glDrawBuffers(attachmentBuffers.size(), attachmentBuffers.data());

        GLuint depthHandle;
        glGenRenderbuffers(1, &depthHandle);
        glBindRenderbuffer(GL_RENDERBUFFER, depthHandle);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthHandle);

        fbo.handle = GLUINT_TO_HANDLE(fboHandle);
        fbo.depthHandle = GLUINT_TO_HANDLE(depthHandle);
        fbo.attachments = attachments;
        fbo.width = width;
        fbo.height = height;

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error("cannot instantiate framebuffer");
        }

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return fbo;
    }

    void GPU::DestroyFramebufferWithAttachments(Framebuffer fbo) {
        for (auto& attachment : fbo.attachments) {
            GPU::DestroyTexture(attachment);
        }

        GPU::DestroyFramebuffer(fbo);
    }

    void GPU::DestroyFramebuffer(Framebuffer fbo) {
        GLuint fboHandle = (uint32_t) (uint64_t) fbo.handle;
        GLuint depthHandle = (uint32_t) (uint64_t) fbo.depthHandle;
        glDeleteRenderbuffers(1, &depthHandle);
        glDeleteFramebuffers(1, &fboHandle);
    }

    void GPU::BindFramebuffer(std::optional<Framebuffer> fbo) {
        if (fbo.has_value()) {
            glBindFramebuffer(GL_FRAMEBUFFER, (GLuint) (uint64_t) fbo.value().handle);
            glViewport(0, 0, fbo.value().width, fbo.value().height);
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, s_width, s_height);
        }
    }

    ArrayBuffer GPU::GenerateBuffer(size_t size, ArrayBufferType type, ArrayBufferUsage usage) {
        GLuint buffer;
        glGenBuffers(1, &buffer);
        glBindBuffer(InterpretArrayBufferType(type), buffer);
        glBufferData(InterpretArrayBufferType(type), size, nullptr, InterpretArrayBufferUsage(usage));

        ArrayBuffer result;
        result.handle = GLUINT_TO_HANDLE(buffer);
        result.usage = usage;
        result.size = size;
        result.type = type;
        return result;
    }

    void GPU::DestroyBuffer(ArrayBuffer& buffer) {
        GLuint id = HANDLE_TO_GLUINT(buffer.handle);
        glDeleteBuffers(1, &id);
    }

    void GPU::FillBuffer(ArrayBuffer& buffer, size_t offset, size_t size, void* data) {
        GLuint id = HANDLE_TO_GLUINT(buffer.handle);
        glBindBuffer(InterpretArrayBufferType(buffer.type), id);
        glBufferSubData(InterpretArrayBufferType(buffer.type), offset, size, data);
    }

    void GPU::BindBufferBase(ArrayBuffer& buffer, int binding) {
        glBindBuffer(InterpretArrayBufferType(buffer.type), binding);
        glBindBufferBase(InterpretArrayBufferType(buffer.type), binding, HANDLE_TO_GLUINT(buffer.handle));
    }

    Shader GPU::GenerateShader(ShaderType type, std::string name, bool useBinaryCache) {
        GLenum enumType = 0;
        std::string extension = "";
        switch (type) {
            case ShaderType::Vertex: {
                enumType = GL_VERTEX_SHADER;
                extension = ".vert";
                break;
            }
            case ShaderType::Fragment: {
                enumType = GL_FRAGMENT_SHADER;
                extension = ".frag";
                break;
            }
            case ShaderType::Compute: {
                enumType = GL_COMPUTE_SHADER;
                extension = ".compute";
                break;
            }
        }

        static std::string shaderCachePath = GetHomePath() + "/.raster/shader_cache/";

        if (!std::filesystem::exists(GetHomePath() + "/.raster/")) {
            std::filesystem::create_directory(GetHomePath() + "/.raster/");
        }

        std::string vendorCache = std::to_string(RSHash(info.renderer));
        if (!std::filesystem::exists(shaderCachePath)) {
            std::filesystem::create_directory(shaderCachePath);
        } 
        if (!std::filesystem::exists(shaderCachePath + "gl/")) {
            std::filesystem::create_directory(shaderCachePath + "gl/");
        }
        if (!std::filesystem::exists(shaderCachePath + "gl/" + vendorCache + "/")) {
            std::filesystem::create_directory(shaderCachePath + "gl/" + vendorCache + "/");
        }

        std::string code = ReadFile("shaders/gl/" + name + extension);
        std::string codeHash = std::to_string(RSHash(code));

        std::string shaderNameHash = std::to_string(RSHash(name + extension));
        std::string nameHashPath = shaderCachePath + "gl/" + vendorCache + "/" + shaderNameHash + ".hash";
        
        bool mustReplaceBlob = false;

        if (std::filesystem::exists(nameHashPath) && useBinaryCache) {
            std::string savedCodeHash = ReadFile(nameHashPath);
            if (savedCodeHash == codeHash) {
                std::cout << "found cached version of " << name << std::endl;
                std::string programBinaryString = ReadFile(shaderCachePath + "gl/" + vendorCache + "/" + shaderNameHash + ".bin");
                std::vector<GLbyte> programBinary(programBinaryString.begin(), programBinaryString.end());

                GLint formats;
                glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);

                std::vector<GLint> binaryFormats(formats);
                glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, binaryFormats.data());

                GLuint loadedProgram = glCreateProgram();
                glProgramBinary(loadedProgram, (GLenum) binaryFormats[0], programBinary.data(), programBinary.size());

                GLint success;
                glGetProgramiv(loadedProgram, GL_LINK_STATUS, &success);
                if (!success) {
                    glDeleteProgram(loadedProgram);
                    mustReplaceBlob = true;
                    std::cout << "failed to load cached version of " << name << std::endl;
                } else {
                    std::cout << "successfully loaded cached version of " << name << std::endl;
                    return Shader(type, GLUINT_TO_HANDLE(loadedProgram));
                }
            }
        }

        const char* rawCode = code.c_str();
        GLuint program = glCreateShaderProgramv(enumType, 1, &rawCode);
        std::vector<char> log(1024);
        GLsizei length;
        glGetProgramInfoLog(program, 1024, &length, log.data());
        if (length != 0) {
            throw std::runtime_error(std::string(log.data()));
        }

        if (std::filesystem::exists(nameHashPath) && useBinaryCache) {
            std::string savedHash = ReadFile(nameHashPath);
            if (savedHash != codeHash) {
                mustReplaceBlob = true;
            }
        } else {
            mustReplaceBlob = true;
        }

        if (mustReplaceBlob && useBinaryCache) {
            GLint formatCount;
            glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formatCount);

            std::vector<GLint> formats(formatCount);
            glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, formats.data());

            GLint length;
            glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &length);

            GLenum binaryFormats;
            std::vector<GLbyte> programBinary(length);
            glGetProgramBinary(program, length, nullptr, &binaryFormats, programBinary.data());

            WriteFile(nameHashPath, codeHash);
            WriteFile(shaderCachePath + "gl/" + vendorCache + "/" + shaderNameHash + ".bin", std::string(programBinary.begin(), programBinary.end()));
            std::cout << "saving cached program binary of " << name << std::endl;
        }

        return Shader(type, GLUINT_TO_HANDLE(program));
    }

    Pipeline GPU::GeneratePipeline(Shader vertexShader, Shader fragmentShader) {
        GLuint pipeline;
        glGenProgramPipelines(1, &pipeline);
        glBindProgramPipeline(pipeline);
        glUseProgramStages(pipeline, GL_VERTEX_SHADER_BIT, HANDLE_TO_GLUINT(vertexShader.handle));
        glUseProgramStages(pipeline, GL_FRAGMENT_SHADER_BIT, HANDLE_TO_GLUINT(fragmentShader.handle));

        Pipeline result;
        result.vertex = vertexShader;
        result.fragment = fragmentShader;
        result.handle = GLUINT_TO_HANDLE(pipeline);
        return result;
    }

    void GPU::DestroyShader(Shader shader) {
        if (!shader.handle) return;
        if (shaderRegistry.find(shader.handle) != shaderRegistry.end()) {
            shaderRegistry.erase(shader.handle);
        }
        glDeleteProgram(HANDLE_TO_GLUINT(shader.handle));
    }

    void GPU::DestroyPipeline(Pipeline pipeline) {
        if (pipeline.vertex.handle != s_basicShader.handle) DestroyShader(pipeline.vertex);
        DestroyShader(pipeline.fragment);
        DestroyShader(pipeline.compute);
        GLuint handle = HANDLE_TO_GLUINT(pipeline.handle);
        glDeleteProgramPipelines(1, &handle);
    }

    int GPU::GetShaderUniformLocation(Shader shader, std::string name) {
        if (shaderRegistry.find(shader.handle) == shaderRegistry.end()) {
            shaderRegistry[shader.handle] = {};
        }
        auto& uniformsMap = shaderRegistry[shader.handle];
        if (uniformsMap.find(name) != uniformsMap.end()) {
            return uniformsMap[name];
        }
        uniformsMap[name] = glGetUniformLocation(HANDLE_TO_GLUINT(shader.handle), name.c_str());
        return uniformsMap[name];
    }

    void GPU::SetShaderUniform(Shader shader, std::string name, int i) {
        auto location = GetShaderUniformLocation(shader, name);
        if (location < 0) return;
        glProgramUniform1iv(HANDLE_TO_GLUINT(shader.handle), location, 1, &i);
    }

    void GPU::SetShaderUniform(Shader shader, std::string name, glm::vec4 vec) {
        auto location = GetShaderUniformLocation(shader, name);
        if (location < 0) return;
        glProgramUniform4fv(HANDLE_TO_GLUINT(shader.handle), location, 1, glm::value_ptr(vec));
    }

    void GPU::SetShaderUniform(Shader shader, std::string name, glm::vec3 vec) {
        auto location = GetShaderUniformLocation(shader, name);
        if (location < 0) return;
        glProgramUniform3fv(HANDLE_TO_GLUINT(shader.handle), location, 1, glm::value_ptr(vec));
    }

    void GPU::SetShaderUniform(Shader shader, std::string name, glm::vec2 vec) {
        auto location = GetShaderUniformLocation(shader, name);
        if (location < 0) return;
        glProgramUniform2fv(HANDLE_TO_GLUINT(shader.handle), location, 1, glm::value_ptr(vec));
    }

    void GPU::SetShaderUniform(Shader shader, std::string name, float f) {
        auto location = GetShaderUniformLocation(shader, name);
        if (location < 0) return;
        glProgramUniform1fv(HANDLE_TO_GLUINT(shader.handle), location, 1, &f);
    }

    void GPU::SetShaderUniform(Shader shader, std::string name, glm::mat4 mat) {
        auto location = GetShaderUniformLocation(shader, name);
        if (location < 0) return;
        glProgramUniformMatrix4fv(HANDLE_TO_GLUINT(shader.handle), location, 1, GL_FALSE, &mat[0][0]);
    }

    void GPU::BindPipeline(Pipeline pipeline) {
        // glUseProgram(0);
        glBindProgramPipeline(HANDLE_TO_GLUINT(pipeline.handle));
    }

    void GPU::ClearFramebuffer(float r, float g, float b, float a) {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GPU::BlitFramebuffer(Framebuffer base, Texture texture, int attachment) {
        glCopyImageSubData(HANDLE_TO_GLUINT(texture.handle), GL_TEXTURE_2D, 0, 0, 0, 0,
                           HANDLE_TO_GLUINT(base.attachments[attachment].handle), GL_TEXTURE_2D, 0, 0, 0, 0, base.width, base.height, 1);
    }

    void GPU::BlitTexture(Texture base, Texture blit) {
        glCopyImageSubData(HANDLE_TO_GLUINT(blit.handle), GL_TEXTURE_2D, 0, 0, 0, 0,
                           HANDLE_TO_GLUINT(base.handle), GL_TEXTURE_2D, 0, 0, 0, 0, base.width, base.height, 1);
    }

    Sampler GPU::GenerateSampler() {
        GLuint sampler;
        glGenSamplers(1, &sampler);

        Sampler result;
        result.handle = GLUINT_TO_HANDLE(sampler);

        SetSamplerTextureFilteringMode(result, TextureFilteringOperation::Minify, TextureFilteringMode::Linear);
        SetSamplerTextureFilteringMode(result, TextureFilteringOperation::Minify, TextureFilteringMode::Linear);

        SetSamplerTextureWrappingMode(result, TextureWrappingAxis::S, TextureWrappingMode::Repeat);
        SetSamplerTextureWrappingMode(result, TextureWrappingAxis::T, TextureWrappingMode::Repeat);

        return result;
    }

    void GPU::BindSampler(std::optional<Sampler> sampler, int unit) {
        glBindSampler(unit, HANDLE_TO_GLUINT((sampler.value_or(0)).handle));
    }

    void GPU::SetSamplerTextureFilteringMode(Sampler& sampler, TextureFilteringOperation operation, TextureFilteringMode mode) {
        glSamplerParameteri(HANDLE_TO_GLUINT(sampler.handle), InterpretTextureFilteringOperation(operation), InterpretTextureFilteringMode(mode));

        if (operation == TextureFilteringOperation::Magnify) {
            sampler.magnifyMode = mode;
        } else {
            sampler.minifyMode = mode;
        }
    }

    void GPU::SetSamplerTextureWrappingMode(Sampler& sampler, TextureWrappingAxis axis, TextureWrappingMode mode) {
        glSamplerParameteri(HANDLE_TO_GLUINT(sampler.handle), InterpretTextureWrappingAxis(axis), InterpretTextureWrappingMode(mode));

        if (axis == TextureWrappingAxis::S) {
            sampler.sMode = mode;
        } else {
            sampler.tMode = mode;
        }
    }

    void GPU::DestroySampler(Sampler& sampler) {
        GLuint handle = HANDLE_TO_GLUINT(sampler.handle);
        glDeleteSamplers(1, &handle);
        sampler = Sampler();
    }

    void GPU::DrawArrays(int count) {
        glDrawArrays(GL_TRIANGLES, 0, count);
    }

    void GPU::ReadPixels(int x, int y, int w, int h, int channels, TexturePrecision texturePrecision, void* data) {
        GLenum precision = GL_UNSIGNED_BYTE;
        if (texturePrecision == TexturePrecision::Half) precision = GL_HALF_FLOAT;
        if (texturePrecision == TexturePrecision::Full) precision = GL_FLOAT;
        glReadPixels(x, y, w, h, InterpretTextureChannels(channels), precision, data);
    }

    void GPU::Terminate() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow((GLFWwindow*) info.display);
        glfwTerminate();
    }

    void GPU::SetWindowTitle(std::string title) {
        s_targetTitle.Set(title);
    }

    std::string GPU::GetShadersPath() {
        return "shaders/gl/";
    }

    void* GPU::GetImGuiContext() {
        return ImGui::GetCurrentContext();
    }

    void* GPU::GetNFDWindowHandle(void* window) {
        NFD_GetNativeWindowFromGLFWWindow((GLFWwindow*) info.display, (nfdwindowhandle_t*) window);
        return window;
    }
}