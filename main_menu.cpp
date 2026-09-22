#include "main_menu.h"
#include "imgui/imgui.h"
#include "tuner_ui.h"
#include "channel_list.h"

enum MenuState {
    MENU_HOME,
    MENU_TUNER,
    MENU_CHANNELS,
    MENU_NETWORK,
    MENU_SYSTEM
};

static MenuState g_currentState = MENU_HOME;
static int home_selected_idx = 0;
static bool g_trigger_exit = false;

void MainMenu_Init() {
    ChannelList_Init();
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
    
    const char* items[] = { "Live TV Channel List", "Tuner Setup / Signal Finder", "Network Configuration", "System Settings", "Standby / Restart / Exit" };
    for (int i = 0; i < 5; i++) {
        // If the window is appearing (e.g. startup or returning from Tuner), force focus to the active item
        if (home_selected_idx == i && ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }

        // Pass 'false' for selected state so we don't get a persistent second blue bar
        if (ImGui::Selectable(items[i], false, 0, ImVec2(0, 50))) {
            if (i == 0) g_currentState = MENU_CHANNELS;
            if (i == 1) g_currentState = MENU_TUNER;
            if (i == 2) g_currentState = MENU_NETWORK;
            if (i == 3) g_currentState = MENU_SYSTEM;
            if (i == 4) g_trigger_exit = true; // Trigger exit from menu item!
        }
        
        // Track the currently focused item via D-Pad so we can restore it later
        if (ImGui::IsItemFocused()) {
            home_selected_idx = i;
        }
    }
    
    ImGui::EndChild();
}

bool MainMenu_Render() {
    bool keep_running = true;
    ImGuiIO& io = ImGui::GetIO();
    
    // Global Back/Exit handler
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        // If the popup is open, ESC closes it (handled inside BeginPopupModal)
        // Otherwise, handle Back/Exit navigation
        if (!ImGui::IsPopupOpen("Exit Confirmation")) {
            if (g_currentState != MENU_HOME) {
                g_currentState = MENU_HOME;
            } else {
                g_trigger_exit = true;
            }
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
    else if (g_currentState == MENU_CHANNELS) {
        ImVec2 listSize(1400, 800);
        ImGui::SetCursorPos(ImVec2((io.DisplaySize.x - listSize.x) * 0.5f, (io.DisplaySize.y - listSize.y) * 0.5f));
        ImGui::BeginChild("ChannelListChild", listSize, true, ImGuiWindowFlags_NoScrollbar);
        ChannelList_Render();
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

    // Process Exit Trigger ONCE
    if (g_trigger_exit) {
        ImGui::OpenPopup("Exit Confirmation");
        g_trigger_exit = false; // Reset trigger
    }

    // Center the popup
    ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    
    if (ImGui::BeginPopupModal("Exit Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to exit to Enigma2 Live TV?\n\n");
        ImGui::Separator();
        
        ImGui::SetItemDefaultFocus();
        if (ImGui::Button("Cancel", ImVec2(150, 50)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::SameLine();
        
        // Make OK button red
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        if (ImGui::Button("Exit", ImVec2(150, 50))) {
            keep_running = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(2);
        
        ImGui::EndPopup();
    }

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    
    return keep_running;
}
