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

void MainMenu_Init() {
    // We already initialize TunerUI in plugin.cpp, but we can do setup here
}

void RenderHome() {
    ImVec2 buttonSize(300, 100);
    
    // Center the buttons
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImGui::SetCursorPos(ImVec2((windowSize.x - buttonSize.x) * 0.5f, windowSize.y * 0.3f));

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    
    if (ImGui::Button("Tuner Setup", buttonSize)) {
        g_currentState = MENU_TUNER;
    }
    
    ImGui::SetCursorPosX((windowSize.x - buttonSize.x) * 0.5f);
    ImGui::Spacing(); ImGui::Spacing();
    
    if (ImGui::Button("Network Configuration", buttonSize)) {
        g_currentState = MENU_NETWORK;
    }
    
    ImGui::SetCursorPosX((windowSize.x - buttonSize.x) * 0.5f);
    ImGui::Spacing(); ImGui::Spacing();
    
    if (ImGui::Button("System Settings", buttonSize)) {
        g_currentState = MENU_SYSTEM;
    }
    
    ImGui::PopStyleVar();
}

void MainMenu_Render() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Create a full-screen window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                             ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoCollapse | 
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBringToFrontOnFocus;

    // Use a semi-transparent dark background for the "GlassHD" look over the TV stream
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.08f, 0.85f));
    
    ImGui::Begin("Enigma2 Main Menu", nullptr, flags);
    
    // Header
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // TODO: Load larger font
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "ENIGMA2 IMGUI SYSTEM");
    ImGui::Separator();
    ImGui::PopFont();
    ImGui::Spacing(); ImGui::Spacing();

    // Render current state
    if (g_currentState == MENU_HOME) {
        RenderHome();
    } 
    else {
        // Shared back button for all sub-menus
        if (ImGui::Button("< Back to Home", ImVec2(200, 50))) {
            g_currentState = MENU_HOME;
        }
        ImGui::Spacing();
        
        if (g_currentState == MENU_NETWORK) {
            ImGui::Text("Network configuration UI coming soon...");
        }
        else if (g_currentState == MENU_SYSTEM) {
            ImGui::Text("System settings UI coming soon...");
        }
    }

    // Footer / Infobar
    ImGui::SetCursorPosY(io.DisplaySize.y - 50);
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Navigation: D-Pad | Select: OK | Exit: EXIT/POWER");

    ImGui::End();
    ImGui::PopStyleColor();
    
    // Render sub-menus as independent floating windows OVER the full-screen background
    if (g_currentState == MENU_TUNER) {
        TunerUI_Render();
    }
}
