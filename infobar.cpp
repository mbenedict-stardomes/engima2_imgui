#include "infobar.h"
#include "imgui/imgui.h"
#include <string>

#include <time.h>



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
    
    ImVec2 infobarSize(1600, 200);
    ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - infobarSize.x) * 0.5f, io.DisplaySize.y - infobarSize.y - 50));
    ImGui::SetNextWindowSize(infobarSize);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.05f, 0.15f, 0.85f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.4f, 0.8f, 0.5f));
    
    ImGui::Begin("InfobarBig", nullptr, flags);
    
    ImGui::Columns(3, "InfobarColumns", false);
    ImGui::SetColumnWidth(0, 250);
    ImGui::SetColumnWidth(1, 950);
    ImGui::SetColumnWidth(2, 400);
    
    // Left: Picon
    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 20, ImGui::GetCursorPosY() + 30));
    ImVec2 pStart = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddRectFilled(pStart, ImVec2(pStart.x + 180, pStart.y + 110), IM_COL32(255, 255, 255, 40), 8.0f);
    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 50, ImGui::GetCursorPosY() + 40));
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "PICON");
    
    ImGui::NextColumn();
    
    // Middle: EPG Info
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.2f, 1.0f), "%s", channel_name ? channel_name : "Unknown Channel");
    ImGui::PopFont();
    
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "NOW  News Update");
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
    ImGui::Text("Global coverage of today's top stories.");
    ImGui::PopStyleColor();
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "NEXT Documentary");

    ImGui::NextColumn();
    
    // Right: Telemetry
    time_t rawtime;
    struct tm * timeinfo;
    char timeBuffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", timeinfo);
    
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 250);
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", timeBuffer);
    ImGui::PopFont();
    
    ImGui::Spacing();
    float bar_width = io.DisplaySize.x * 0.15f;
    float bar_height = io.DisplaySize.y * 0.015f;
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
    
    ImGui::Text("SNR: %d%%", g_snr);
    ImGui::SameLine(ImGui::GetWindowWidth() * 0.65f);
    ImGui::ProgressBar((float)g_snr / 100.0f, ImVec2(bar_width, bar_height), "");
    
    ImGui::Text("AGC: %d%%", g_agc);
    ImGui::SameLine(ImGui::GetWindowWidth() * 0.65f);
    ImGui::ProgressBar((float)g_agc / 100.0f, ImVec2(bar_width, bar_height), "");
    
    ImGui::PopStyleColor();
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    ImGui::Text("BER: %d", g_ber);
    ImGui::SameLine(ImGui::GetWindowWidth() * 0.65f);
    ImGui::ProgressBar(0.0f, ImVec2(bar_width, bar_height), "");
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", g_ext_telemetry.c_str());
    
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "1920x1080i  |  DVB  |  FTA");

    ImGui::Columns(1);
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}


void Infobar_RenderSmall(const char* channel_name) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.7f, io.DisplaySize.y * 0.15f), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - (io.DisplaySize.x * 0.7f)) * 0.5f, io.DisplaySize.y - (io.DisplaySize.y * 0.18f)), ImGuiCond_Always);
    
    ImGui::Begin("SmallInfobar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetWindowPos();
    ImVec2 s = ImGui::GetWindowSize();
    draw_list->AddRectFilled(p, ImVec2(p.x + s.x, p.y + s.y), IM_COL32(20, 25, 35, 230), 15.0f);
    draw_list->AddRect(p, ImVec2(p.x + s.x, p.y + s.y), IM_COL32(100, 150, 255, 100), 15.0f, 0, 2.0f);
    
    ImGui::SetCursorPos(ImVec2(s.x * 0.05f, s.y * 0.2f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.2f, 1.0f), "Playing: %s", channel_name ? channel_name : "Unknown");
    ImGui::SetCursorPos(ImVec2(s.x * 0.05f, s.y * 0.5f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::Text("NOW: %s", g_epg_now_name.c_str());
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    
    ImGui::SetCursorPos(ImVec2(s.x * 0.05f, s.y * 0.75f));
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "NEXT: %s", g_epg_next_name.c_str());
    
    ImGui::End();
}
