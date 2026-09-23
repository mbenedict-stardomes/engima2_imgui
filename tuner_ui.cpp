#include "tuner_ui.h"
#include "imgui/imgui.h"
#include "pugixml.hpp"
#include <iostream>
#include <cmath>

std::vector<Satellite> g_satellites;
int current_sat_idx = 0;
int current_ts_idx = 0;

void TunerUI_Init() {
    pugi::xml_document doc;
    // Use the native Enigma2 XML path on the STB
    pugi::xml_parse_result result = doc.load_file("/etc/tuxbox/satellites.xml");
    
    if (!result) {
        std::cerr << "Failed to load /etc/tuxbox/satellites.xml: " << result.description() << "\n";
        return;
    }
    
    pugi::xml_node satellites = doc.child("satellites");
    for (pugi::xml_node sat_node = satellites.child("sat"); sat_node; sat_node = sat_node.next_sibling("sat")) {
        Satellite sat;
        sat.name = sat_node.attribute("name").value();
        sat.position = sat_node.attribute("position").as_int();
        sat.flags = sat_node.attribute("flags").as_int();
        
        for (pugi::xml_node ts_node = sat_node.child("transponder"); ts_node; ts_node = ts_node.next_sibling("transponder")) {
            Transponder ts;
            ts.frequency = ts_node.attribute("frequency").as_int();
            ts.symbol_rate = ts_node.attribute("symbol_rate").as_int();
            ts.polarization = ts_node.attribute("polarization").as_int();
            ts.fec_inner = ts_node.attribute("fec_inner").as_int();
            ts.system = ts_node.attribute("system").as_int();
            ts.modulation = ts_node.attribute("modulation").as_int();
            sat.transponders.push_back(ts);
        }
        g_satellites.push_back(sat);
    }
    std::cout << "Loaded " << g_satellites.size() << " satellites.\n";
}

void TunerUI_Render() {
    if (g_satellites.empty()) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: satellites.xml not loaded.");
        return;
    }
    
    ImGui::Text("Manual Scan / Signal Finder");
    ImGui::Separator();
    ImGui::Spacing();
    
    if (ImGui::IsWindowAppearing()) {
        ImGui::SetKeyboardFocusHere();
    }
    
    // Satellite Combo
    const char* preview_sat = g_satellites[current_sat_idx].name.c_str();
    if (ImGui::BeginCombo("Satellite", preview_sat)) {
        for (int i = 0; i < g_satellites.size(); ++i) {
            bool is_selected = (current_sat_idx == i);
            if (ImGui::Selectable(g_satellites[i].name.c_str(), is_selected)) {
                current_sat_idx = i;
                current_ts_idx = 0; // reset transponder
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    
    // Transponder Combo
    if (current_sat_idx >= 0 && current_sat_idx < g_satellites.size() && !g_satellites[current_sat_idx].transponders.empty()) {
        const auto& sat = g_satellites[current_sat_idx];
        const auto& curr_ts = sat.transponders[current_ts_idx];
        
        char ts_preview[128];
        snprintf(ts_preview, sizeof(ts_preview), "%d MHz / %d SR", curr_ts.frequency / 1000, curr_ts.symbol_rate / 1000);
        
        if (ImGui::BeginCombo("Transponder", ts_preview)) {
            for (int i = 0; i < sat.transponders.size(); ++i) {
                const auto& ts = sat.transponders[i];
                char label[128];
                snprintf(label, sizeof(label), "%d MHz / %d SR / Pol: %d", ts.frequency / 1000, ts.symbol_rate / 1000, ts.polarization);
                
                bool is_selected = (current_ts_idx == i);
                if (ImGui::Selectable(label, is_selected)) {
                    current_ts_idx = i;
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        
        ImGui::Spacing();
        ImGui::Text("Transponder Details:");
        ImGui::BulletText("Frequency: %d MHz", curr_ts.frequency / 1000);
        ImGui::BulletText("Symbol Rate: %d KS/s", curr_ts.symbol_rate / 1000);
        ImGui::BulletText("Polarization: %d", curr_ts.polarization);
        ImGui::BulletText("FEC: %d", curr_ts.fec_inner);
        ImGui::BulletText("System: %s", curr_ts.system == 0 ? "DVB-S" : "DVB-S2");
    } else {
        ImGui::TextDisabled("No transponders available for this satellite.");
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    extern int g_snr, g_agc, g_ber;
    
    ImGui::Text("Signal Telemetry");
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
    char buf[32];
    sprintf(buf, "SNR %d%%", g_snr);
    ImGui::ProgressBar(g_snr / 100.0f, ImVec2(-1.0f, 0.0f), buf);
    ImGui::PopStyleColor();
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
    sprintf(buf, "AGC %d%%", g_agc);
    ImGui::ProgressBar(g_agc / 100.0f, ImVec2(-1.0f, 0.0f), buf);
    ImGui::PopStyleColor();
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    sprintf(buf, "BER %d", g_ber);
    ImGui::ProgressBar(g_ber > 100 ? 1.0f : g_ber / 100.0f, ImVec2(-1.0f, 0.0f), buf);
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // CONSTELLATION DIAGRAM
    ImGui::Text("Constellation Diagram (IQ Plot)");
    ImVec2 canvas_p = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImVec2(300, 300);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Draw background
    draw_list->AddRectFilled(canvas_p, ImVec2(canvas_p.x + canvas_size.x, canvas_p.y + canvas_size.y), IM_COL32(20, 20, 20, 255));
    draw_list->AddRect(canvas_p, ImVec2(canvas_p.x + canvas_size.x, canvas_p.y + canvas_size.y), IM_COL32(255, 255, 255, 100));
    
    // Draw grid lines (I and Q axis)
    draw_list->AddLine(ImVec2(canvas_p.x + canvas_size.x/2, canvas_p.y), ImVec2(canvas_p.x + canvas_size.x/2, canvas_p.y + canvas_size.y), IM_COL32(255, 255, 255, 100));
    draw_list->AddLine(ImVec2(canvas_p.x, canvas_p.y + canvas_size.y/2), ImVec2(canvas_p.x + canvas_size.x, canvas_p.y + canvas_size.y/2), IM_COL32(255, 255, 255, 100));
    
    // Calculate noise spread based on SNR (0 SNR = 100% spread, 100 SNR = 0% spread)
    float noise_radius = (100.0f - g_snr) * 1.5f;
    if (noise_radius < 5.0f) noise_radius = 5.0f;
    
    // Determine modulation (curr_ts.modulation: 1=QPSK, 2=8PSK)
    int modulation = 1; // Default QPSK
    if (current_sat_idx >= 0 && current_sat_idx < g_satellites.size() && !g_satellites[current_sat_idx].transponders.empty()) {
        modulation = g_satellites[current_sat_idx].transponders[current_ts_idx].modulation;
    }
    
    int num_clusters = (modulation == 2) ? 8 : 4; // 8PSK vs QPSK
    ImU32 point_col = IM_COL32(50, 255, 50, 255);
    if (g_snr < 50) point_col = IM_COL32(255, 255, 50, 255);
    if (g_snr < 30) point_col = IM_COL32(255, 50, 50, 255);
    
    float center_x = canvas_p.x + canvas_size.x / 2.0f;
    float center_y = canvas_p.y + canvas_size.y / 2.0f;
    float cluster_radius = 80.0f;
    
    // Render 100 points per cluster with random noise
    for (int c = 0; c < num_clusters; c++) {
        float angle = (c * (360.0f / num_clusters) + 45.0f) * 3.14159f / 180.0f;
        float cx = center_x + cosf(angle) * cluster_radius;
        float cy = center_y + sinf(angle) * cluster_radius;
        
        for (int p = 0; p < 50; p++) {
            // Very simple pseudo-random noise
            float rx = ((float)rand() / RAND_MAX - 0.5f) * noise_radius;
            float ry = ((float)rand() / RAND_MAX - 0.5f) * noise_radius;
            draw_list->AddCircleFilled(ImVec2(cx + rx, cy + ry), 1.5f, point_col);
        }
    }
    
    ImGui::Dummy(canvas_size); // Reserve space for custom drawing
    
    ImGui::Spacing();
    if (ImGui::Button("Start Blindscan", ImVec2(200, 50))) {
        // Do blindscan
    }
    ImGui::SameLine();
    if (ImGui::Button("Manual Scan", ImVec2(200, 50))) {
        // Do manual scan
    }
    ImGui::SameLine();
    if (ImGui::Button("Auto Scan", ImVec2(200, 50))) {
        // Do auto scan
    }
}
