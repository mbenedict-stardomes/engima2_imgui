#include "main_menu.h"
#include "imgui/imgui.h"
#include "tuner_ui.h"

enum MenuState {
    MENU_HOME,
    MENU_TUNER,
    MENU_NETWORK,
    MENU_SYSTEM
};

static MenuState g_currentState = MENU_HOME;
static int home_selected_idx = 0;

void MainMenu_Init() {
}

void RenderHome() {
    ImVec2 windowSize = ImGui::GetWindowSize();
    
    // Classic Enigma2 Center Dialog
    ImVec2 dialogSize(800, 600);
    ImGui::SetCursorPos(ImVec2((windowSize.x - dialogSize.x) * 0.5f, (windowSize.y - dialogSize.y) * 0.5f));
    
    ImGui::BeginChild("HomeMenuDialog", dialogSize, true, ImGuiWindowFlags_NoScrollbar);
    
    // Header
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
    ImGui::Text("Main Menu");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();
    
    const char* items[] = { "Tuner Setup / Signal Finder", "Network Configuration", "System Settings", "Standby / Restart" };
    for (int i = 0; i < 4; i++) {
        bool is_selected = (home_selected_idx == i);
        if (ImGui::Selectable(items[i], is_selected, 0, ImVec2(0, 50))) {
            home_selected_idx = i;
            if (i == 0) g_currentState = MENU_TUNER;
            if (i == 1) g_currentState = MENU_NETWORK;
            if (i == 2) g_currentState = MENU_SYSTEM;
        }
        if (is_selected) {
            ImGui::SetItemDefaultFocus();
        }
    }
    
    ImGui::EndChild();
}

bool MainMenu_Render() {
    bool keep_running = true;
    ImGuiIO& io = ImGui::GetIO();
    
    // Global Back/Exit handler
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        if (g_currentState != MENU_HOME) {
            g_currentState = MENU_HOME;
        } else {
            keep_running = false; // Tell plugin.cpp to exit
        }
    }
    
    // Create a full-screen background
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                             ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoCollapse | 
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.70f)); // Darker background
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    
    ImGui::Begin("Enigma2 Main Menu", nullptr, flags);
    
    // Header Logo
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "ENIGMA2 IMGUI PROTOTYPE");
    ImGui::Separator();
    
    if (g_currentState == MENU_HOME) {
        RenderHome();
    } 
    else if (g_currentState == MENU_TUNER) {
        // Embed the Tuner UI exactly in the center
        ImVec2 tunerSize(1200, 700);
        ImGui::SetCursorPos(ImVec2((io.DisplaySize.x - tunerSize.x) * 0.5f, (io.DisplaySize.y - tunerSize.y) * 0.5f));
        ImGui::BeginChild("TunerChild", tunerSize, true, ImGuiWindowFlags_NoScrollbar);
        TunerUI_Render();
        ImGui::EndChild();
    }
    else {
        ImGui::Text("Coming soon...");
    }

    // Classic Enigma2 Footer (Color Buttons)
    ImGui::SetCursorPosY(io.DisplaySize.y - 60);
    ImGui::Separator();
    
    // Render color buttons
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1)); ImGui::Text("  Red  "); ImGui::PopStyleColor(); ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1, 0, 1)); ImGui::Text(" Green "); ImGui::PopStyleColor(); ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1)); ImGui::Text(" Yellow"); ImGui::PopStyleColor(); ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 1, 1)); ImGui::Text(" Blue  "); ImGui::PopStyleColor(); ImGui::SameLine();
    
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "     | Navigation: D-Pad | Select: OK | Exit: EXIT/POWER");

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    
    return keep_running;
}
