#include "channel_list.h"
#include "imgui/imgui.h"
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

void ChannelList_Init() {
    if (!g_bouquets.empty()) return;
    
    // Mock Data
    Bouquet fav;
    fav.name = "Favourites (TV)";
    fav.channels.push_back({1, "BBC One HD", 
        {"19:00 - 20:00", "BBC News at Six", "National and international news."},
        {"20:00 - 21:00", "The Graham Norton Show", "Talk show featuring celebrity guests."}});
    fav.channels.push_back({2, "BBC Two HD", 
        {"19:30 - 20:30", "Mastermind", "Classic quiz show."},
        {"20:30 - 21:30", "Top Gear", "Motoring magazine."}});
    fav.channels.push_back({3, "ITV 1 HD", 
        {"19:00 - 19:30", "Emmerdale", "Soap opera."},
        {"19:30 - 20:00", "Coronation Street", "Soap opera."}});
    fav.channels.push_back({4, "Channel 4 HD", 
        {"19:00 - 20:00", "Channel 4 News", "News and current affairs."},
        {"20:00 - 21:00", "The Great British Bake Off", "Baking competition."}});
    fav.channels.push_back({5, "Sky Sports Main Event", 
        {"18:00 - 21:00", "Live Premier League", "Arsenal vs Chelsea."},
        {"21:00 - 22:00", "Post Match Analysis", "Review of the game."}});
        
    Bouquet movies;
    movies.name = "Movies";
    for (int i=1; i<=10; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Sky Cinema %d", i);
        movies.channels.push_back({100+i, buf, 
            {"20:00 - 22:00", "Blockbuster Movie", "Action packed thriller."},
            {"22:00 - 00:00", "Late Night Comedy", "Stand-up special."}});
    }
    
    g_bouquets.push_back(fav);
    g_bouquets.push_back(movies);
}

void ChannelList_Render() {
    if (g_bouquets.empty()) return;
    
    Bouquet& bq = g_bouquets[current_bouquet_idx];
    
    // Split layout: 60% channels, 40% EPG info
    ImGui::Columns(2, "ChannelListColumns", false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.6f);
    
    // --- LEFT PANE: Channel List ---
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
    ImGui::Text("%s", bq.name.c_str());
    ImGui::PopStyleColor();
    ImGui::Separator();
    
    // We use a Child window for scrolling
    ImGui::BeginChild("ChannelScrollingRegion", ImVec2(0, ImGui::GetWindowHeight() - 150));
    for (int i = 0; i < bq.channels.size(); ++i) {
        if (current_channel_idx == i && ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }
        
        char label[128];
        snprintf(label, sizeof(label), "%3d   %s", bq.channels[i].number, bq.channels[i].name.c_str());
        
        if (ImGui::Selectable(label, false, 0, ImVec2(0, 40))) {
            // When OK is pressed, we could switch channel and close the menu
            // For now just select
            current_channel_idx = i;
        }
        
        if (ImGui::IsItemFocused()) {
            current_channel_idx = i;
        }
    }
    ImGui::EndChild();
    
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
    
    // Handle Left/Right D-Pad to switch bouquets
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        if (current_bouquet_idx > 0) {
            current_bouquet_idx--;
            current_channel_idx = 0;
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        if (current_bouquet_idx < g_bouquets.size() - 1) {
            current_bouquet_idx++;
            current_channel_idx = 0;
        }
    }
}
