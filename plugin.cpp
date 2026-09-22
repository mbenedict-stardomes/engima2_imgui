#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <queue>
#include <mutex>
#include <string>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "main_menu.h"
#include "tuner_ui.h"
#include "evdev_input.h"
#include "channel_list.h"

static std::queue<std::string> g_action_queue;
static std::mutex g_action_mutex;

extern "C" void SendImGuiAction(const char* action) {
    std::lock_guard<std::mutex> lock(g_action_mutex);
    g_action_queue.push(action);
}

uint64_t get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

char g_selected_service_ref[512] = {0};
bool g_exit_and_play = false;

extern "C" void SetChannelDataXML(const char* xml_data) {
    ChannelList_LoadXML(xml_data);
}

extern "C" const char* StartImGuiPlugin() {
    g_selected_service_ref[0] = '\0';
    g_exit_and_play = false;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!eglInitialize(display, NULL, NULL)) return "";

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

    TunerUI_Init();
    MainMenu_Init();

    uint64_t last_time = get_time_ms();

    bool keep_running = true;

    while (keep_running) {
        uint64_t current_time = get_time_ms();
        io.DeltaTime = (float)(current_time - last_time) / 1000.0f;
        if (io.DeltaTime <= 0.0f) io.DeltaTime = 0.016f;
        last_time = current_time;

        // Process keys from Python Enigma2 ActionMap
        {
            std::lock_guard<std::mutex> lock(g_action_mutex);
            while (!g_action_queue.empty()) {
                std::string action = g_action_queue.front();
                g_action_queue.pop();
                
                ImGuiKey imgui_key = ImGuiKey_None;
                if (action == "up") imgui_key = ImGuiKey_UpArrow;
                else if (action == "down") imgui_key = ImGuiKey_DownArrow;
                else if (action == "left") imgui_key = ImGuiKey_LeftArrow;
                else if (action == "right") imgui_key = ImGuiKey_RightArrow;
                else if (action == "ok") imgui_key = ImGuiKey_Enter;
                else if (action == "cancel" || action == "exit") imgui_key = ImGuiKey_Escape;
                
                if (imgui_key != ImGuiKey_None) {
                    io.AddKeyEvent(imgui_key, true);
                    io.AddKeyEvent(imgui_key, false);
                }
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        if (!MainMenu_Render()) {
            keep_running = false;
        }

        if (g_exit_and_play) {
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
    
    // Clear the screen buffer completely so the UI doesn't hang around on exit
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(display, surface);
    printf("EGL Buffer cleared.\n");

    ImGui_ImplOpenGL3_Shutdown();
    printf("ImGui OpenGL3 shut down.\n");
    
    ImGui::DestroyContext();
    printf("ImGui Context destroyed.\n");

    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    printf("EGL MakeCurrent detached.\n");
    
    eglDestroySurface(display, surface);
    printf("EGL Surface destroyed.\n");
    
    eglDestroyContext(display, context);
    printf("EGL Context destroyed.\n");
    
    eglTerminate(display);
    printf("EGL Terminated.\n");

    printf("Returning service ref string: '%s'\n", g_selected_service_ref);
    return g_selected_service_ref;
}
