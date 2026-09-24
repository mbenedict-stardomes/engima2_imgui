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
    ImGuiIO& io = ImGui::GetIO();
    
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "ADVANCED TUNER MANAGEMENT");
    ImGui::Separator();
    ImGui::Spacing();
    
    if (ImGui::BeginTabBar("TunerTabs", ImGuiTabBarFlags_None)) {
        
        // ==========================================
        // TAB 1: HARDWARE CONFIGURATION
        // ==========================================
        if (ImGui::BeginTabItem("Hardware Setup")) {
            ImGui::Spacing();
            ImGui::Text("Configure Physical Tuner Interfaces:");
            ImGui::Spacing();
            
            static int selected_tuner = 0;
            ImGui::RadioButton("Tuner A", &selected_tuner, 0); ImGui::SameLine();
            ImGui::RadioButton("Tuner B", &selected_tuner, 1);
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            static int tuner_mode = 0;
            ImGui::Text("Configuration Mode:");
            ImGui::Combo("##Mode", &tuner_mode, "Simple\0Advanced\0Equal to\0Loop through to\0Nothing connected\0");
            
            if (tuner_mode == 0) { // Simple
                static int simple_mode = 0;
                ImGui::Combo("Mode", &simple_mode, "Single\0Toneburst\0DiSEqC A/B\0DiSEqC A/B/C/D\0Positioner\0");
                
                static int dvb_type = 0;
                ImGui::Combo("DVB Type", &dvb_type, "DVB-S2 (Satellite)\0DVB-T2 (Terrestrial)\0DVB-C (Cable)\0");
                
                if (dvb_type == 0) {
                    if (g_satellites.size() > 0) {
                        static int lnb_sat = 0;
                        if (ImGui::BeginCombo("Satellite", g_satellites[lnb_sat].name.c_str())) {
                            for (int i = 0; i < g_satellites.size(); ++i) {
                                bool is_selected = (lnb_sat == i);
                                if (ImGui::Selectable(g_satellites[i].name.c_str(), is_selected)) lnb_sat = i;
                            }
                            ImGui::EndCombo();
                        }
                    }
                    static bool send_diseqc = false;
                    ImGui::Checkbox("Send DiSEqC", &send_diseqc);
                } else if (dvb_type == 1) {
                    static int terr_region = 0;
                    ImGui::Combo("Region", &terr_region, "Europe, Middle East, Africa: DVB-T/T2\0Australia: DVB-T\0");
                } else {
                    static int cable_region = 0;
                    ImGui::Combo("Region", &cable_region, "Europe DVB-C\0");
                }
            }
            
            ImGui::Spacing();
            if (ImGui::Button("Save Configuration", ImVec2(200, 40))) {
                // IPC hook to NimManager goes here
            }
            
            ImGui::EndTabItem();
        }
        
        // ==========================================
        // TAB 2: TRANSPONDER MANAGEMENT & SCANNING
        // ==========================================
        if (ImGui::BeginTabItem("TP & Scanning")) {
            ImGui::Spacing();
            ImGui::Columns(2, "ScanCols", false);
            ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.6f);
            
            ImGui::Text("Transponder Management:");
            
            // Satellite Combo
            const char* preview_sat = g_satellites.empty() ? "None" : g_satellites[current_sat_idx].name.c_str();
            if (ImGui::BeginCombo("Satellite", preview_sat)) {
                for (int i = 0; i < g_satellites.size(); ++i) {
                    bool is_selected = (current_sat_idx == i);
                    if (ImGui::Selectable(g_satellites[i].name.c_str(), is_selected)) {
                        current_sat_idx = i;
                        current_ts_idx = 0; // reset transponder
                    }
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
                        if (ImGui::Selectable(label, is_selected)) current_ts_idx = i;
                    }
                    ImGui::EndCombo();
                }
                
                ImGui::Spacing();
                if (ImGui::Button("Add TP")) {} ImGui::SameLine();
                if (ImGui::Button("Edit TP")) {} ImGui::SameLine();
                if (ImGui::Button("Remove TP")) {}
            }
            
            ImGui::NextColumn();
            ImGui::Text("Scan Operations:");
            ImGui::Spacing();
            if (ImGui::Button("Automatic Scan (Full)", ImVec2(-1, 50))) {}
            if (ImGui::Button("Manual Scan (Current TP)", ImVec2(-1, 50))) {}
            if (ImGui::Button("Hardware Blindscan", ImVec2(-1, 50))) {}
            
            ImGui::Columns(1);
            ImGui::EndTabItem();
        }
        
        // ==========================================
        // TAB 3: SATELLITE SIGNAL FINDER
        // ==========================================
        if (ImGui::BeginTabItem("Satellite Finder")) {
            ImGui::Spacing();
            
            extern int g_snr, g_agc, g_ber;
            ImGui::Columns(2, "SatFinderCols", false);
            ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.45f);
            
            ImGui::Text("Tuner Lock Metrics");
            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
            char buf[32]; sprintf(buf, "SNR %d%%", g_snr);
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
            
            ImGui::NextColumn();
            
            // CONSTELLATION DIAGRAM
            ImGui::Text("Constellation Diagram (IQ Plot)");
            ImVec2 canvas_p = ImGui::GetCursorScreenPos();
            ImVec2 canvas_size = ImVec2(ImGui::GetWindowWidth() * 0.35f, ImGui::GetWindowWidth() * 0.35f);
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            
            draw_list->AddRectFilled(canvas_p, ImVec2(canvas_p.x + canvas_size.x, canvas_p.y + canvas_size.y), IM_COL32(20, 20, 20, 255));
            draw_list->AddRect(canvas_p, ImVec2(canvas_p.x + canvas_size.x, canvas_p.y + canvas_size.y), IM_COL32(255, 255, 255, 100));
            draw_list->AddLine(ImVec2(canvas_p.x + canvas_size.x/2, canvas_p.y), ImVec2(canvas_p.x + canvas_size.x/2, canvas_p.y + canvas_size.y), IM_COL32(255, 255, 255, 100));
            draw_list->AddLine(ImVec2(canvas_p.x, canvas_p.y + canvas_size.y/2), ImVec2(canvas_p.x + canvas_size.x, canvas_p.y + canvas_size.y/2), IM_COL32(255, 255, 255, 100));
            
            float noise_radius = (100.0f - g_snr) * 1.5f;
            if (noise_radius < 5.0f) noise_radius = 5.0f;
            
            int num_clusters = 4; // Default QPSK
            ImU32 point_col = IM_COL32(50, 255, 50, 255);
            if (g_snr < 50) point_col = IM_COL32(255, 255, 50, 255);
            if (g_snr < 30) point_col = IM_COL32(255, 50, 50, 255);
            
            float center_x = canvas_p.x + canvas_size.x / 2.0f;
            float center_y = canvas_p.y + canvas_size.y / 2.0f;
            float cluster_radius = canvas_size.x * 0.25f;
            
            for (int c = 0; c < num_clusters; c++) {
                float angle = (c * (360.0f / num_clusters) + 45.0f) * 3.14159f / 180.0f;
                float cx = center_x + cosf(angle) * cluster_radius;
                float cy = center_y + sinf(angle) * cluster_radius;
                
                for (int p = 0; p < 50; p++) {
                    float rx = ((float)rand() / RAND_MAX - 0.5f) * noise_radius;
                    float ry = ((float)rand() / RAND_MAX - 0.5f) * noise_radius;
                    draw_list->AddCircleFilled(ImVec2(cx + rx, cy + ry), 1.5f, point_col);
                }
            }
            ImGui::Dummy(canvas_size);
            ImGui::Columns(1);
            
            ImGui::EndTabItem();
        }
        
        // ==========================================
        // TAB 4: TERRESTRIAL FINDER
        // ==========================================
        if (ImGui::BeginTabItem("Terrestrial Finder")) {
            ImGui::Spacing();
            ImGui::Text("DVB-T / DVB-T2 Alignment");
            ImGui::Separator();
            ImGui::Spacing();
            
            static int terr_freq = 610000;
            static int terr_bw = 8; // MHz
            
            ImGui::InputInt("Frequency (kHz)", &terr_freq, 1000, 10000);
            ImGui::SliderInt("Bandwidth (MHz)", &terr_bw, 6, 8);
            
            ImGui::Spacing();
            if (ImGui::Button("Lock Tuner to Frequency", ImVec2(300, 40))) {}
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            extern int g_snr, g_agc;
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.6f, 0.9f, 1.0f));
            char buf[32]; sprintf(buf, "SNR %d%%", g_snr);
            ImGui::ProgressBar(g_snr / 100.0f, ImVec2(-1.0f, 40.0f), buf);
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
            sprintf(buf, "AGC %d%%", g_agc);
            ImGui::ProgressBar(g_agc / 100.0f, ImVec2(-1.0f, 40.0f), buf);
            ImGui::PopStyleColor();
            
            ImGui::EndTabItem();
        }
        
        // ==========================================
        // TAB 5: CABLE FINDER
        // ==========================================
        if (ImGui::BeginTabItem("Cable Finder")) {
            ImGui::Spacing();
            ImGui::Text("DVB-C Diagnostics");
            ImGui::Separator();
            ImGui::Spacing();
            
            static int cab_freq = 410000;
            static int cab_sr = 6900;
            static int cab_qam = 2; // 0=16,1=32,2=64,3=128,4=256
            
            ImGui::InputInt("Frequency (kHz)", &cab_freq, 1000, 10000);
            ImGui::InputInt("Symbol Rate (KS/s)", &cab_sr, 100, 1000);
            ImGui::Combo("Modulation", &cab_qam, "QAM16\0QAM32\0QAM64\0QAM128\0QAM256\0");
            
            ImGui::Spacing();
            if (ImGui::Button("Lock Tuner to Frequency", ImVec2(300, 40))) {}
            
            ImGui::Spacing();
            
            extern int g_snr, g_agc;
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.9f, 0.4f, 0.2f, 1.0f));
            char buf[32]; sprintf(buf, "SNR %d%%", g_snr);
            ImGui::ProgressBar(g_snr / 100.0f, ImVec2(-1.0f, 40.0f), buf);
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
            sprintf(buf, "AGC %d%%", g_agc);
            ImGui::ProgressBar(g_agc / 100.0f, ImVec2(-1.0f, 40.0f), buf);
            ImGui::PopStyleColor();
            
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
}
