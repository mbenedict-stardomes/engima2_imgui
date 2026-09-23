#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "main_menu.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <queue>
#include <string>
#include <mutex>
#include <time.h>

extern void MainMenu_Init();
extern bool MainMenu_Render();
extern void ChannelList_Init();
extern void ChannelList_LoadXML(const char* xml_data);
extern void TunerUI_Init();
extern void Infobar_Init();

extern bool g_trigger_exit;
extern int g_currentState;

static std::queue<std::string> g_action_queue;
static std::mutex g_action_mutex;

static std::string g_pending_playback;
static std::mutex g_playback_mutex;

extern "C" void SendImGuiAction(const char* action) {
    std::lock_guard<std::mutex> lock(g_action_mutex);
    g_action_queue.push(action); printf("[ImGui] Received Action: %s\n", action);
}

extern "C" void TriggerPlayback(const char* ref_str) {
    std::lock_guard<std::mutex> lock(g_playback_mutex);
    g_pending_playback = ref_str;
}

extern "C" const char* GetPendingPlayback() {
    std::lock_guard<std::mutex> lock(g_playback_mutex);
    if (g_pending_playback.empty()) return nullptr;
    
    static char buf[256];
    strncpy(buf, g_pending_playback.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf)-1] = '\0';
    g_pending_playback.clear();
    return buf;
}

uint64_t get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

extern "C" void SetChannelDataXML(const char* xml_data) {
    ChannelList_LoadXML(xml_data);
}

extern "C" const char* StartImGuiPlugin() {
    g_trigger_exit = false;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!eglInitialize(display, NULL, NULL)) return nullptr;

    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);

    EGLSurface surface = eglCreateWindowSurface(display, config, 0, NULL);
    
    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    
    eglMakeCurrent(display, surface, surface, context);
    printf("EGL Context Created. Bootstrapping ImGui inside Python plugin...\n");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1920, 1080);
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    
    // Scale up fonts for 1080p TV
    ImFontConfig font_cfg;
    font_cfg.SizePixels = 32.0f;
    io.Fonts->AddFontDefault(&font_cfg);
    
    ImGui::StyleColorsDark();
    ImGui_ImplOpenGL3_Init("#version 100");

    MainMenu_Init();
    ChannelList_Init();
    TunerUI_Init();
    Infobar_Init();

    uint64_t last_time = get_time_ms();
    bool keep_running = true;

    while (keep_running) {
        // 1. Calculate DeltaTime BEFORE NewFrame
        uint64_t current_time = get_time_ms();
        io.DeltaTime = (current_time - last_time) / 1000.0f;
        if (io.DeltaTime <= 0.0f) io.DeltaTime = 0.016f;
        last_time = current_time;

        // 2. Process keys BEFORE NewFrame so ImGui sees the state transition
        {
            std::lock_guard<std::mutex> lock(g_action_mutex);
            while (!g_action_queue.empty()) {
                std::string action = g_action_queue.front();
                g_action_queue.pop();
                

                if (action.rfind("vol_", 0) == 0) {
                    extern void TriggerVolumeOverlay(int vol);
                    int v = std::stoi(action.substr(4));
                    TriggerVolumeOverlay(v);
                    continue;
                }


                if (action.rfind("telemetry|", 0) == 0) {
                    extern void UpdateTelemetry(int snr, int agc, int ber);
                    int snr = 0, agc = 0, ber = 0;
                    sscanf(action.c_str(), "telemetry|%d|%d|%d", &snr, &agc, &ber);
                    UpdateTelemetry(snr, agc, ber);
                    continue;
                }
                if (action.rfind("epg_now|", 0) == 0) {
                    extern void UpdateEPGNow(const char* name, const char* desc);
                    size_t p1 = action.find('|', 8);
                    if (p1 != std::string::npos) {
                        std::string name = action.substr(8, p1 - 8);
                        std::string desc = action.substr(p1 + 1);
                        UpdateEPGNow(name.c_str(), desc.c_str());
                    }
                    continue;
                }
                if (action.rfind("epg_next|", 0) == 0) {
                    extern void UpdateEPGNext(const char* name, const char* desc);
                    size_t p1 = action.find('|', 9);
                    if (p1 != std::string::npos) {
                        std::string name = action.substr(9, p1 - 9);
                        std::string desc = action.substr(p1 + 1);
                        UpdateEPGNext(name.c_str(), desc.c_str());
                    }
                    continue;
                }


                if (action == "menu") {
                    g_currentState = MENU_HOME;
                    continue;
                }

                if (action == "info") {
                    extern uint64_t g_infobar_timer;
                    extern uint64_t get_time_ms();
                    if (g_currentState == MENU_LIVETV) {
                        g_currentState = MENU_INFOBAR_SMALL; 
                        g_infobar_timer = get_time_ms();
                    } else if (g_currentState == MENU_INFOBAR_SMALL) {
                        g_currentState = MENU_INFOBAR_BIG;
                        g_infobar_timer = get_time_ms();
                    } else if (g_currentState == MENU_INFOBAR_BIG) {
                        g_currentState = MENU_LIVETV;
                    } else {
                        g_currentState = MENU_INFOBAR_SMALL;
                        g_infobar_timer = get_time_ms();
                    }
                    continue;
                }
                if (action == "channels") {
                    g_currentState = MENU_CHANNELS;
                    continue;
                }
                
                ImGuiKey imgui_key = ImGuiKey_None;
                if (action == "up") imgui_key = ImGuiKey_UpArrow;
                else if (action == "down") imgui_key = ImGuiKey_DownArrow;
                else if (action == "left") imgui_key = ImGuiKey_LeftArrow;
                else if (action == "right") imgui_key = ImGuiKey_RightArrow;
                else if (action == "ok") imgui_key = ImGuiKey_Enter;
                else if (action == "cancel" || action == "exit") imgui_key = ImGuiKey_Escape;
                else if (action == "info") imgui_key = ImGuiKey_I;
                
                if (imgui_key != ImGuiKey_None) {
                    io.AddKeyEvent(imgui_key, true);
                    io.AddKeyEvent(imgui_key, false);
                }
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
        
        if (g_trigger_exit) keep_running = false;
        
        if (!MainMenu_Render()) {
            keep_running = false;
        }

        ImGui::Render();
        glViewport(0, 0, 1920, 1080);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        eglSwapBuffers(display, surface);
        
        usleep(16000); // ~60fps
    }

    printf("Shutting down ImGui and returning to Enigma2...\n");
    
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(display, surface);
    printf("EGL Buffer cleared.\n");

    ImGui_ImplOpenGL3_Shutdown();
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

    return nullptr;
}
