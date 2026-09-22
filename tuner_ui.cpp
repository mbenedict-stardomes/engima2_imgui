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
    
    // Mock Signal Finder Meters
    ImGui::Text("Signal Telemetry");
    
    // Animate a fake signal lock
    float time = ImGui::GetTime();
    float snr = 0.8f + 0.05f * sinf(time * 3.0f);
    float agc = 0.9f + 0.02f * cosf(time * 2.0f);
    float ber = 0.0f; // BER ideally 0
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
    ImGui::ProgressBar(snr, ImVec2(-1.0f, 0.0f), "SNR (Signal to Noise Ratio)");
    ImGui::PopStyleColor();
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.6f, 0.1f, 1.0f));
    ImGui::ProgressBar(agc, ImVec2(-1.0f, 0.0f), "AGC (Auto Gain Control)");
    ImGui::PopStyleColor();
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));
    ImGui::ProgressBar(ber, ImVec2(-1.0f, 0.0f), "BER (Bit Error Rate)");
    ImGui::PopStyleColor();
    
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
