#include "channel_list.h"
#include "imgui/imgui.h"
#include "pugixml.hpp"
#include <vector>
#include <string>

struct EPGEvent {
    std::string time;
    std::string title;
    std::string description;
};

struct Channel {
    int number;
    std::string name;
    std::string ref;
    EPGEvent current_epg;
    EPGEvent next_epg;
};

struct Bouquet {
    std::string name;
    std::vector<Channel> channels;
};

static std::vector<Bouquet> g_bouquets;
static int current_bouquet_idx = 0;
static int current_channel_idx = 0;

extern char g_selected_service_ref[512];
extern bool g_exit_and_play;

// Needed for Master UI Architecture transitions
extern void SetCurrentChannelName(const char* name);
extern "C" void TriggerPlayback(const char* ref_str);
extern uint64_t g_infobar_timer;
extern uint64_t get_time_ms();
extern int g_currentState; // 0=MENU_LIVETV, 1=MENU_HOME, 2=MENU_INFOBAR_SMALL, 3=MENU_INFOBAR_BIG, 4=MENU_TUNER, 5=MENU_CHANNELS, 6=MENU_NETWORK, 7=MENU_SYSTEM

void ChannelList_LoadXML(const char* xml_data) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml_data);
    if (!result) {
        printf("Failed to parse channel XML from Python: %s\n", result.description());
        return;
    }
    
    g_bouquets.clear();
    
    for (pugi::xml_node bouquet_node : doc.child("bouquets").children("bouquet")) {
        Bouquet bq;
        bq.name = bouquet_node.attribute("name").value();
        
        for (pugi::xml_node channel_node : bouquet_node.children("channel")) {
            Channel ch;
            ch.number = channel_node.attribute("number").as_int();
            ch.name = channel_node.attribute("name").value();
            ch.ref = channel_node.attribute("ref").value();
            
            // For now, mock EPG
            ch.current_epg = {"NOW", "Program Info", "Description of current show."};
            ch.next_epg = {"NEXT", "Next Program Info", "Description of next show."};
            
            bq.channels.push_back(ch);
        }
        g_bouquets.push_back(bq);
    }
    
    current_bouquet_idx = 0;
    current_channel_idx = 0;
}

void ChannelList_Init() {
    // We only load mock data if g_bouquets is empty (e.g. testing without Python)
    if (!g_bouquets.empty()) return;
    
    const char* mock_xml = 
    "<bouquets>\n"
    "    <bouquet name=\"Favourites (TV)\">\n"
    "        <channel number=\"1\" name=\"BBC One HD\" ref=\"1:0:1:1:1:1000:0:0:0:0:\" />\n"
    "        <channel number=\"2\" name=\"BBC Two HD\" ref=\"1:0:1:2:1:1000:0:0:0:0:\" />\n"
    "        <channel number=\"3\" name=\"ITV 1 HD\" ref=\"1:0:1:3:1:1000:0:0:0:0:\" />\n"
    "        <channel number=\"4\" name=\"Channel 4 HD\" ref=\"1:0:1:4:1:1000:0:0:0:0:\" />\n"
    "        <channel number=\"5\" name=\"Sky Sports\" ref=\"1:0:1:5:1:1000:0:0:0:0:\" />\n"
    "    </bouquet>\n"
    "    <bouquet name=\"Movies\">\n"
    "        <channel number=\"101\" name=\"Sky Cinema 1\" ref=\"1:0:1:6:1:1000:0:0:0:0:\" />\n"
    "        <channel number=\"102\" name=\"Sky Cinema 2\" ref=\"1:0:1:7:1:1000:0:0:0:0:\" />\n"
    "    </bouquet>\n"
    "</bouquets>\n";
    
    ChannelList_LoadXML(mock_xml);
}

void ChannelList_Render() {
    if (g_bouquets.empty()) return;
    
    Bouquet& bq = g_bouquets[current_bouquet_idx];
    
    // Split layout: 60% channels, 40% EPG info
    ImGui::Columns(2, "ChannelListColumns", false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.6f);
    
    // --- LEFT PANE: Channel List ---
    static int pending_bouquet_switch = -1;
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        if (current_bouquet_idx > 0) pending_bouquet_switch = current_bouquet_idx - 1;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        if (current_bouquet_idx < g_bouquets.size() - 1) pending_bouquet_switch = current_bouquet_idx + 1;
    }
    
    if (ImGui::BeginTabBar("BouquetTabs", ImGuiTabBarFlags_FittingPolicyScroll)) {
        for (int b = 0; b < g_bouquets.size(); ++b) {
            ImGuiTabItemFlags flags = 0;
            if (pending_bouquet_switch == b) {
                flags |= ImGuiTabItemFlags_SetSelected;
                if (b == g_bouquets.size() - 1 || pending_bouquet_switch == 0) pending_bouquet_switch = -1; // reset when edge reached
            }
            
            if (ImGui::BeginTabItem(g_bouquets[b].name.c_str(), nullptr, flags)) {
                if (current_bouquet_idx != b) {
                    current_bouquet_idx = b;
                    current_channel_idx = 0;
                }
                
                ImGui::BeginChild("ChannelScrollingRegion", ImVec2(0, ImGui::GetWindowHeight() - 150));
                for (int i = 0; i < g_bouquets[b].channels.size(); ++i) {
                    if (current_channel_idx == i && ImGui::IsWindowAppearing()) {
                        ImGui::SetKeyboardFocusHere();
                    }
                    
                    char label[128];
                    snprintf(label, sizeof(label), "%3d   %s", g_bouquets[b].channels[i].number, g_bouquets[b].channels[i].name.c_str());
                    
                    if (ImGui::Selectable(label, false, 0, ImVec2(0, 40))) {
                        current_channel_idx = i;
                        TriggerPlayback(g_bouquets[b].channels[i].ref.c_str());
                        SetCurrentChannelName(g_bouquets[b].channels[i].name.c_str());
                        g_infobar_timer = get_time_ms();
                        g_currentState = 2; // MENU_INFOBAR_SMALL
                    }
                    
                    if (ImGui::IsItemFocused()) {
                        current_channel_idx = i;
                        if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Space)) {
                            TriggerPlayback(g_bouquets[b].channels[i].ref.c_str());
                            SetCurrentChannelName(g_bouquets[b].channels[i].name.c_str());
                            g_infobar_timer = get_time_ms();
                            g_currentState = 2; // MENU_INFOBAR_SMALL
                        }
                    }
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
        if (pending_bouquet_switch != -1) pending_bouquet_switch = -1;
    }
    
    ImGui::NextColumn();
    
    // --- RIGHT PANE: EPG Info ---
    if (current_channel_idx >= 0 && current_channel_idx < bq.channels.size()) {
        Channel& ch = bq.channels[current_channel_idx];
        
        ImGui::BeginChild("EPGRegion", ImVec2(0, ImGui::GetWindowHeight() - 150));
        
        // Large Channel Name
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // using default loaded font
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
        ImGui::Text("%s", ch.name.c_str());
        ImGui::PopStyleColor();
        ImGui::PopFont();
        
        ImGui::Separator();
        ImGui::Spacing();
        
        // Now EPG
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "NOW  %s", ch.current_epg.time.c_str());
        ImGui::TextWrapped("%s", ch.current_epg.title.c_str());
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::TextWrapped("%s", ch.current_epg.description.c_str());
        ImGui::PopStyleColor();
        
        ImGui::Spacing(); ImGui::Spacing();
        
        // Next EPG
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "NEXT %s", ch.next_epg.time.c_str());
        ImGui::TextWrapped("%s", ch.next_epg.title.c_str());
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::TextWrapped("%s", ch.next_epg.description.c_str());
        ImGui::PopStyleColor();
        
        ImGui::EndChild();
    }
    ImGui::Columns(1);
    
    ImGui::Separator();
    
    // Render color buttons specific to channel list
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1)); ImGui::Text("  All  "); ImGui::PopStyleColor(); ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1, 0, 1)); ImGui::Text(" Satellites "); ImGui::PopStyleColor(); ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1)); ImGui::Text(" Provider"); ImGui::PopStyleColor(); ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 1, 1)); ImGui::Text(" Favourites  "); ImGui::PopStyleColor();
    
}
