#include "infobar.h"
#include "imgui/imgui.h"
#include <string>
#include <time.h>

static int g_snr = 85;
static int g_agc = 70;
static int g_ber = 0;

void Infobar_Init() {
}

void Infobar_Render() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Position at the bottom of the screen (leaving 50px margin)
    ImVec2 infobarSize(1600, 200);
    ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - infobarSize.x) * 0.5f, io.DisplaySize.y - infobarSize.y - 50));
    ImGui::SetNextWindowSize(infobarSize);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                             ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoCollapse | 
                             ImGuiWindowFlags_NoSavedSettings;

    // GlassHD style transparent dark background
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.05f, 0.15f, 0.85f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.4f, 0.8f, 0.5f));
    
    ImGui::Begin("Infobar", nullptr, flags);
    
    // Split into 3 columns: Left (Picon), Middle (Info), Right (Telemetry)
    ImGui::Columns(3, "InfobarColumns", false);
    ImGui::SetColumnWidth(0, 250);
    ImGui::SetColumnWidth(1, 950);
    ImGui::SetColumnWidth(2, 400);
    
    // --- COLUMN 0: Picon ---
    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 20, ImGui::GetCursorPosY() + 30));
    // Draw a placeholder box for Picon
    ImVec2 pStart = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddRectFilled(pStart, ImVec2(pStart.x + 180, pStart.y + 110), IM_COL32(255, 255, 255, 40), 8.0f);
    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 50, ImGui::GetCursorPosY() + 40));
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "PICON");
    
    ImGui::NextColumn();
    
    // --- COLUMN 1: EPG Info ---
    // Channel Number and Name
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.2f, 1.0f), "1  BBC One HD");
    ImGui::PopFont();
    
    ImGui::Spacing();
    
    // Now/Next EPG
    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "19:00 - 20:00  BBC News at Six");
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
    ImGui::Text("National and international news.");
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "20:00 - 21:00  The Graham Norton Show");

    ImGui::NextColumn();
    
    // --- COLUMN 2: Telemetry & Tech Info ---
    time_t rawtime;
    struct tm * timeinfo;
    char timeBuffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", timeinfo);
    
    // Large Clock
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 250);
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", timeBuffer);
    ImGui::PopFont();
    
    ImGui::Spacing();
    
    // Telemetry Progress Bars
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.8f, 0.2f, 1.0f)); // Green bars
    
    char buf[32];
    snprintf(buf, sizeof(buf), "SNR: %d%%", g_snr);
    ImGui::ProgressBar((float)g_snr / 100.0f, ImVec2(300, 20), buf);
    
    snprintf(buf, sizeof(buf), "AGC: %d%%", g_agc);
    ImGui::ProgressBar((float)g_agc / 100.0f, ImVec2(300, 20), buf);
    
    ImGui::PopStyleColor();
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // Red bar for BER
    snprintf(buf, sizeof(buf), "BER: %d", g_ber);
    ImGui::ProgressBar(0.0f, ImVec2(300, 20), buf);
    ImGui::PopStyleColor();
    
    // Resolution Info
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "1920x1080i  |  DVB-S2  |  FTA");

    ImGui::Columns(1);
    
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}
