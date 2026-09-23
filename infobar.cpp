#include "infobar.h"
#include "imgui/imgui.h"
#include <string>

#include <time.h>

std::string g_ext_telemetry = "";



#include <string>

#include <fstream>
#include <sstream>

std::string GetProcVal(const char* path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) return "?";
    std::string val;
    ifs >> val;
    return val;
}


extern "C" {
    void UpdateExtTelemetry(const char* ext) {
        if (ext) g_ext_telemetry = ext;
    }
}

void DrawMediaSpecs() {
    std::string xres = GetProcVal("/proc/stb/vmpeg/0/xres");
    std::string yres = GetProcVal("/proc/stb/vmpeg/0/yres");
    std::string prog = GetProcVal("/proc/stb/vmpeg/0/progressive");
    std::string fps = GetProcVal("/proc/stb/vmpeg/0/framerate");
    
    char fps_disp[16] = "";
    if (fps != "?") {
        int f = std::stoi(fps);
        snprintf(fps_disp, sizeof(fps_disp), "%d fps", f / 1000);
    }
    
    std::string scan = (prog == "1") ? "p" : "i";
    
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Video: %sx%s%s %s", xres.c_str(), yres.c_str(), scan.c_str(), fps_disp);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), " | Audio: DVB");
}

int g_snr = 0, g_agc = 0, g_ber = 0;

std::string g_epg_now_name = "Loading...";
std::string g_epg_now_desc = "";
std::string g_epg_next_name = "Loading...";
std::string g_epg_next_desc = "";

extern "C" void UpdateTelemetry(int snr, int agc, int ber) {
    g_snr = snr; g_agc = agc; g_ber = ber;
}
extern "C" void UpdateEPGNow(const char* name, const char* desc) {
    g_epg_now_name = name ? name : "";
    g_epg_now_desc = desc ? desc : "";
}
extern "C" void UpdateEPGNext(const char* name, const char* desc) {
    g_epg_next_name = name ? name : "";
    g_epg_next_desc = desc ? desc : "";
}

void Infobar_Init() {
}

void Infobar_RenderBig(const char* channel_name) {
    ImGuiIO& io = ImGui::GetIO();
    
    // Stack it right on top of the Small Infobar
    float bottom_padding = io.DisplaySize.y * 0.05f;
    float small_height = io.DisplaySize.y * 0.12f;
    
    ImVec2 s(io.DisplaySize.x * 0.8f, io.DisplaySize.y * 0.18f);
    ImGui::SetNextWindowSize(s, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - s.x) * 0.5f, io.DisplaySize.y - s.y - bottom_padding - small_height - 10), ImGuiCond_Always);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.15f, 0.25f, 0.95f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.6f, 1.0f, 0.6f));
    
    ImGui::Begin("InfobarSecondary", nullptr, flags);
    
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "EXTENDED EVENT DETAILS");
    ImGui::Separator();
    
    ImGui::TextWrapped("%s", g_epg_now_desc.c_str());
    ImGui::Spacing();
    ImGui::Spacing();
    
    ImGui::Separator();
    
    // Telemetry & Specs Footer
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "TUNER DATA:  %s", g_ext_telemetry.c_str());
    
    ImGui::Spacing();
    DrawMediaSpecs();
    
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}
