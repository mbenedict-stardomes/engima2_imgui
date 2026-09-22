#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "main_menu.h"
#include "tuner_ui.h"
#include "evdev_input.h"

uint64_t get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

extern "C" void StartImGuiPlugin() {
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!eglInitialize(display, NULL, NULL)) return;

    EGLint attr[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };

    EGLConfig config;
    EGLint num_config;
    eglChooseConfig(display, attr, &config, 1, &num_config);

    EGLint ctx_attr[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctx_attr);
    EGLSurface surface = eglCreateWindowSurface(display, config, 0, NULL);
    eglMakeCurrent(display, surface, surface, context);

    printf("EGL Context Created. Bootstrapping ImGui inside Python plugin...\n");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = NULL;
    io.DisplaySize = ImVec2(1920, 1080);
    
    // Enable Keyboard Navigation for Remote Control mappings
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Load native Enigma2 font for 1080p display
    ImFont* font = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/Roboto-Bold.ttf", 26.0f);
    if (!font) {
        printf("Failed to load Roboto-Bold.ttf, falling back to default.\n");
    }
    
    ImGui::StyleColorsDark();

    // Setup OpenGL ES 2.0 backend
    ImGui_ImplOpenGL3_Init("#version 100");

    // Initialize the STB Remote Control Evdev Interface
    Evdev_Init("/dev/input/event0");

    TunerUI_Init();
    MainMenu_Init();

    uint64_t last_time = get_time_ms();

    // For a plugin, we run until the user hits EXIT (handled inside Evdev_Poll)
    bool keep_running = true;

    while (keep_running) {
        uint64_t current_time = get_time_ms();
        io.DeltaTime = (float)(current_time - last_time) / 1000.0f;
        if (io.DeltaTime <= 0.0f) io.DeltaTime = 0.016f;
        last_time = current_time;

        // Feed STB Remote Control inputs to ImGui
        if (!Evdev_Poll_Plugin()) {
            keep_running = false;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        if (!MainMenu_Render()) {
            keep_running = false;
        }

        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        eglSwapBuffers(display, surface);
        usleep(16000); 
    }

    printf("Shutting down ImGui and returning to Enigma2...\n");
    Evdev_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();

    // VERY IMPORTANT for plugins: release the EGL context so Enigma2 can take the screen back!
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface);
    eglDestroyContext(display, context);
    eglTerminate(display);
}
