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
    
    // Spread widgets out globally
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 15));
    
    if (ImGui::BeginTabBar("TunerTabs", ImGuiTabBarFlags_None)) {
        
        // ==========================================
        // TAB 1: HARDWARE CONFIGURATION
        // ==========================================
        if (ImGui::BeginTabItem("Hardware Setup")) {
            ImGui::Spacing(); ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Configure Physical Tuner Interfaces:");
            ImGui::Spacing();
            
            static int selected_tuner = 0;
            ImGui::RadioButton("Tuner A", &selected_tuner, 0); ImGui::SameLine(200);
            ImGui::RadioButton("Tuner B", &selected_tuner, 1);
            
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            
            ImGui::PushItemWidth(io.DisplaySize.x * 0.4f); // Prevent combo boxes from stretching infinitely
            
            static int tuner_mode = 0;
            ImGui::Combo("Configuration Mode", &tuner_mode, "Simple\0Advanced\0Equal to\0Loop through to\0Nothing connected\0");
            
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
            ImGui::PopItemWidth();
            
            ImGui::Spacing(); ImGui::Spacing();
            if (ImGui::Button("Save Configuration", ImVec2(250, 50))) {}
            
            ImGui::EndTabItem();
        }
        
        // ==========================================
        // TAB 2: TRANSPONDER MANAGEMENT
        // ==========================================
        if (ImGui::BeginTabItem("TP Management")) {
            ImGui::Spacing(); ImGui::Spacing();
            
            ImGui::Columns(2, "TPCols", false);
            ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.45f);
            
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Satellite / Network Selection:");
            ImGui::PushItemWidth(-1);
            
            const char* preview_sat = g_satellites.empty() ? "None" : g_satellites[current_sat_idx].name.c_str();
            if (ImGui::BeginCombo("##Satellite", preview_sat)) {
                for (int i = 0; i < g_satellites.size(); ++i) {
                    bool is_selected = (current_sat_idx == i);
                    if (ImGui::Selectable(g_satellites[i].name.c_str(), is_selected)) {
                        current_sat_idx = i;
                        current_ts_idx = 0;
                    }
                }
                ImGui::EndCombo();
            }
            
            ImGui::Spacing(); ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Transponder Selection:");
            if (current_sat_idx >= 0 && current_sat_idx < g_satellites.size() && !g_satellites[current_sat_idx].transponders.empty()) {
                const auto& sat = g_satellites[current_sat_idx];
                const auto& curr_ts = sat.transponders[current_ts_idx];
                
                char ts_preview[128];
                snprintf(ts_preview, sizeof(ts_preview), "%d MHz / %d SR", curr_ts.frequency / 1000, curr_ts.symbol_rate / 1000);
                
                if (ImGui::BeginCombo("##Transponder", ts_preview)) {
                    for (int i = 0; i < sat.transponders.size(); ++i) {
                        const auto& ts = sat.transponders[i];
                        char label[128];
                        snprintf(label, sizeof(label), "%d MHz / %d SR / Pol: %d", ts.frequency / 1000, ts.symbol_rate / 1000, ts.polarization);
                        bool is_selected = (current_ts_idx == i);
                        if (ImGui::Selectable(label, is_selected)) current_ts_idx = i;
                    }
                    ImGui::EndCombo();
                }
                
                ImGui::PopItemWidth();
                ImGui::NextColumn();
                
                // TP Details / Edit form
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Transponder Parameters:");
                ImGui::Separator(); ImGui::Spacing();
                
                ImGui::PushItemWidth(ImGui::GetColumnWidth(0) * 0.5f);
                int mod_freq = curr_ts.frequency / 1000;
                ImGui::InputInt("Frequency (MHz)", &mod_freq);
                
                int mod_sr = curr_ts.symbol_rate / 1000;
                ImGui::InputInt("Symbol Rate (KS/s)", &mod_sr);
                
                static int mod_pol = curr_ts.polarization;
                ImGui::Combo("Polarization", &mod_pol, "Horizontal\0Vertical\0Circular Left\0Circular Right\0");
                
                static int mod_fec = curr_ts.fec_inner;
                ImGui::Combo("FEC Inner", &mod_fec, "Auto\0 1/2\0 2/3\0 3/4\0 5/6\0 7/8\0 8/9\0 3/5\0 4/5\0 9/10\0 None\0");
                
                static int mod_sys = curr_ts.system;
                ImGui::Combo("System (DVB Type)", &mod_sys, "DVB-S\0DVB-S2\0");
                
                static int mod_mod = curr_ts.modulation;
                ImGui::Combo("Modulation", &mod_mod, "Auto\0QPSK\08PSK\016APSK\032APSK\0");
                
                ImGui::PopItemWidth();
                
                ImGui::Spacing(); ImGui::Spacing();
                if (ImGui::Button("Add New TP", ImVec2(io.DisplaySize.x * 0.15f, 60))) {} ImGui::SameLine();
                if (ImGui::Button("Save Changes", ImVec2(io.DisplaySize.x * 0.15f, 60))) {} ImGui::SameLine();
                if (ImGui::Button("Delete TP", ImVec2(io.DisplaySize.x * 0.15f, 60))) {}
            } else {
                ImGui::PopItemWidth();
                ImGui::NextColumn();
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No transponders found.");
            }
            
            ImGui::Columns(1);
            ImGui::EndTabItem();
        }
        
        // ==========================================
        // TAB 3: RECEPTION & SCANNING
        // ==========================================
        if (ImGui::BeginTabItem("Reception / Scan")) {
            ImGui::Spacing(); ImGui::Spacing();
            
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Select Scan Operation:");
            static int scan_type = 0;
            ImGui::RadioButton("Automatic Scan (Full)", &scan_type, 0); ImGui::SameLine(ImGui::GetWindowWidth() * 0.35f);
            ImGui::RadioButton("Manual Scan", &scan_type, 1); ImGui::SameLine(ImGui::GetWindowWidth() * 0.65f);
            ImGui::RadioButton("Blindscan", &scan_type, 2);
            
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            
            ImGui::PushItemWidth(ImGui::GetColumnWidth(0) * 0.5f);
            if (scan_type == 0) {
                static int auto_tuner = 0;
                ImGui::Combo("Tuner", &auto_tuner, "All Tuners\0Tuner A (DVB-S2)\0Tuner B (DVB-T2)\0");
                static bool clear_before = true;
                ImGui::Checkbox("Clear before scan", &clear_before);
            } else if (scan_type == 1) {
                static int manual_tuner = 0;
                ImGui::Combo("Tuner", &manual_tuner, "Tuner A (DVB-S2)\0Tuner B (DVB-T2)\0");
                static int manual_type = 0;
                ImGui::Combo("Type of Scan", &manual_type, "Single Transponder\0Single Satellite\0Multisat\0");
                
                if (manual_type == 0) { // Single TP
                    ImGui::TextDisabled("Select Satellite and Transponder below:");
                    
                    const char* preview_sat = g_satellites.empty() ? "None" : g_satellites[current_sat_idx].name.c_str();
                    if (ImGui::BeginCombo("Satellite", preview_sat)) {
                        for (int i = 0; i < g_satellites.size(); ++i) {
                            if (ImGui::Selectable(g_satellites[i].name.c_str(), current_sat_idx == i)) {
                                current_sat_idx = i;
                                current_ts_idx = 0;
                            }
                        }
                        ImGui::EndCombo();
                    }
                    
                    if (current_sat_idx >= 0 && current_sat_idx < g_satellites.size() && !g_satellites[current_sat_idx].transponders.empty()) {
                        const auto& sat = g_satellites[current_sat_idx];
                        const auto& curr_ts = sat.transponders[current_ts_idx];
                        char ts_preview[128]; snprintf(ts_preview, sizeof(ts_preview), "%d MHz / %d SR", curr_ts.frequency / 1000, curr_ts.symbol_rate / 1000);
                        if (ImGui::BeginCombo("Transponder", ts_preview)) {
                            for (int i = 0; i < sat.transponders.size(); ++i) {
                                const auto& ts = sat.transponders[i];
                                char label[128]; snprintf(label, sizeof(label), "%d MHz / %d SR / Pol: %d", ts.frequency / 1000, ts.symbol_rate / 1000, ts.polarization);
                                if (ImGui::Selectable(label, current_ts_idx == i)) current_ts_idx = i;
                            }
                            ImGui::EndCombo();
                        }
                    }
                } else {
                    ImGui::BeginDisabled();
                    static int dummy_ts = 0;
                    ImGui::Combo("Transponder", &dummy_ts, "All Transponders\0");
                    ImGui::EndDisabled();
                }
            } else if (scan_type == 2) {
                static int blind_tuner = 0;
                ImGui::Combo("Tuner", &blind_tuner, "Tuner A (DVB-S2)\0Tuner B (DVB-T2)\0");
            }
            ImGui::PopItemWidth();
            
            ImGui::Spacing(); ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
            if (ImGui::Button("START SCAN", ImVec2(300, 60))) {}
            ImGui::PopStyleColor();
            
            ImGui::EndTabItem();
        }
        
        // ==========================================
        // TAB 4: SATELLITE SIGNAL FINDER
        // ==========================================
        if (ImGui::BeginTabItem("Satellite Finder")) {
            ImGui::Spacing(); ImGui::Spacing();
            
            ImGui::Columns(2, "SatFinderCols", false);
            ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.45f);
            
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Tune to Satellite & Transponder:");
            ImGui::PushItemWidth(ImGui::GetColumnWidth(0) * 0.5f);
            
            const char* preview_sat = g_satellites.empty() ? "None" : g_satellites[current_sat_idx].name.c_str();
            if (ImGui::BeginCombo("Satellite##Find", preview_sat)) {
                for (int i = 0; i < g_satellites.size(); ++i) {
                    if (ImGui::Selectable(g_satellites[i].name.c_str(), current_sat_idx == i)) {
                        current_sat_idx = i; current_ts_idx = 0;
                    }
                }
                ImGui::EndCombo();
            }
            
            if (current_sat_idx >= 0 && current_sat_idx < g_satellites.size() && !g_satellites[current_sat_idx].transponders.empty()) {
                const auto& sat = g_satellites[current_sat_idx];
                const auto& curr_ts = sat.transponders[current_ts_idx];
                char ts_preview[128]; snprintf(ts_preview, sizeof(ts_preview), "%d MHz / %d SR", curr_ts.frequency / 1000, curr_ts.symbol_rate / 1000);
                
                if (ImGui::BeginCombo("Transponder##Find", ts_preview)) {
                    for (int i = 0; i < sat.transponders.size(); ++i) {
                        const auto& ts = sat.transponders[i];
                        char label[128]; snprintf(label, sizeof(label), "%d MHz / %d SR / Pol: %d", ts.frequency / 1000, ts.symbol_rate / 1000, ts.polarization);
                        if (ImGui::Selectable(label, current_ts_idx == i)) current_ts_idx = i;
                    }
                    ImGui::EndCombo();
                }
                
                ImGui::Spacing();
                // Allow direct editing for signal hunting
                static int hunt_freq = curr_ts.frequency / 1000;
                static int hunt_sr = curr_ts.symbol_rate / 1000;
                ImGui::InputInt("Freq (MHz)", &hunt_freq, 1, 100);
                ImGui::InputInt("Sym (KS/s)", &hunt_sr, 1, 1000);
                
                if (ImGui::Button("Lock Tuner", ImVec2(200, 40))) {}
            }
            ImGui::PopItemWidth();
            
            ImGui::NextColumn();
            
            extern int g_snr, g_agc, g_ber;
            
            // CONSTELLATION DIAGRAM
            ImGui::Spacing(); ImGui::Spacing();
            const char* iq_title = "Constellation Diagram\n(IQ Plot)";
            ImVec2 txt_size = ImGui::CalcTextSize(iq_title);
            float avail_txt = ImGui::GetContentRegionAvail().x;
            if (avail_txt > txt_size.x) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail_txt - txt_size.x) * 0.5f);
            ImGui::Text("%s", iq_title);
            ImVec2 canvas_p = ImGui::GetCursorScreenPos();
            // Center the canvas horizontally within the column
            float avail = ImGui::GetContentRegionAvail().x;
            float canvas_w = ImGui::GetWindowWidth() * 0.25f;
            if (avail > canvas_w) {
                canvas_p.x += (avail - canvas_w) * 0.5f;
                ImGui::SetCursorScreenPos(canvas_p);
            }
            // Reduce size from 0.35f to 0.25f to fit screen
            ImVec2 canvas_size = ImVec2(ImGui::GetWindowWidth() * 0.25f, ImGui::GetWindowWidth() * 0.25f);
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            
            draw_list->AddRectFilled(canvas_p, ImVec2(canvas_p.x + canvas_size.x, canvas_p.y + canvas_size.y), IM_COL32(20, 20, 20, 255));
            draw_list->AddRect(canvas_p, ImVec2(canvas_p.x + canvas_size.x, canvas_p.y + canvas_size.y), IM_COL32(255, 255, 255, 100));
            draw_list->AddLine(ImVec2(canvas_p.x + canvas_size.x/2, canvas_p.y), ImVec2(canvas_p.x + canvas_size.x/2, canvas_p.y + canvas_size.y), IM_COL32(255, 255, 255, 100));
            draw_list->AddLine(ImVec2(canvas_p.x, canvas_p.y + canvas_size.y/2), ImVec2(canvas_p.x + canvas_size.x, canvas_p.y + canvas_size.y/2), IM_COL32(255, 255, 255, 100));
            
            // Dynamically scale noise to keep clusters tight (max 12% of canvas, min 1%)
            float max_noise = canvas_size.x * 0.12f;
            float noise_radius = ((100.0f - g_snr) / 100.0f) * max_noise;
            if (noise_radius < canvas_size.x * 0.01f) noise_radius = canvas_size.x * 0.01f;
            
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
            
            ImGui::Spacing(); ImGui::Spacing();
            ImGui::Text("Live Lock Metrics (Tuned TP)");
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
            char buf[32]; sprintf(buf, "SNR %d%%", g_snr);
            ImGui::ProgressBar(g_snr / 100.0f, ImVec2(-1.0f, 40.0f), buf);
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
            sprintf(buf, "AGC %d%%", g_agc);
            ImGui::ProgressBar(g_agc / 100.0f, ImVec2(-1.0f, 40.0f), buf);
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            sprintf(buf, "BER %d", g_ber);
            ImGui::ProgressBar(g_ber > 100 ? 1.0f : g_ber / 100.0f, ImVec2(-1.0f, 40.0f), buf);
            ImGui::PopStyleColor();
            
            ImGui::EndTabItem();
        }
        
        // ==========================================
        // TAB 5: TERRESTRIAL FINDER
        // ==========================================
        if (ImGui::BeginTabItem("Terrestrial Finder")) {
            ImGui::Spacing(); ImGui::Spacing();
            ImGui::Text("DVB-T / DVB-T2 Alignment");
            ImGui::Separator(); ImGui::Spacing();
            
            ImGui::Columns(2, "TerrFinderCols", false);
            ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.45f);
            
            ImGui::PushItemWidth(ImGui::GetColumnWidth(0) * 0.5f);
            
            static int terr_channel = 21;
            if (ImGui::InputInt("Channel (VHF/UHF)", &terr_channel, 1, 5)) {
                // Auto update frequency based on EU DVB-T standard (Ch21 = 474MHz, 8MHz bandwidth)
                if (terr_channel >= 21 && terr_channel <= 69) {
                    // Update external static freq variable! Wait, we use a static variable inside the function.
                    // To update it from here, we have it declared below.
                }
            }
            
            static int terr_freq = 474000;
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                terr_freq = 474000 + (terr_channel - 21) * 8000;
            }
            ImGui::InputInt("Frequency (kHz)", &terr_freq, 1000, 8000);
            
            // Cross sync channel from freq
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                terr_channel = 21 + ((terr_freq - 474000) / 8000);
            }
            
            static int terr_bw = 8;
            ImGui::SliderInt("Bandwidth (MHz)", &terr_bw, 6, 8);
            
            static int terr_fec = 0;
            ImGui::Combo("FEC High", &terr_fec, "Auto\0 1/2\0 2/3\0 3/4\0 5/6\0 7/8\0");
            
            static int terr_gi = 0;
            ImGui::Combo("Guard Interval", &terr_gi, "Auto\0 1/4\0 1/8\0 1/16\0 1/32\0 1/128\0 19/128\0 19/256\0");
            
            static int terr_mod = 0;
            ImGui::Combo("Modulation", &terr_mod, "Auto\0 QPSK\0 QAM16\0 QAM64\0 QAM256\0");
            
            ImGui::PopItemWidth();
            
            ImGui::Spacing();
            if (ImGui::Button("Lock Tuner to Terrestrial", ImVec2(300, 40))) {}
            
            ImGui::NextColumn();
            
            extern int g_snr, g_agc;
            
            // CONSTELLATION DIAGRAM
            ImGui::Spacing(); ImGui::Spacing();
            const char* qam_title = "Constellation Diagram\n(QAM64 Plot)";
            ImVec2 txt_size2 = ImGui::CalcTextSize(qam_title);
            float avail_txt2 = ImGui::GetContentRegionAvail().x;
            if (avail_txt2 > txt_size2.x) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail_txt2 - txt_size2.x) * 0.5f);
            ImGui::Text("%s", qam_title);
            ImVec2 canvas_p = ImGui::GetCursorScreenPos();
            float avail = ImGui::GetContentRegionAvail().x;
            float canvas_w = ImGui::GetWindowWidth() * 0.25f;
            if (avail > canvas_w) {
                canvas_p.x += (avail - canvas_w) * 0.5f;
                ImGui::SetCursorScreenPos(canvas_p);
            }
            ImVec2 canvas_size = ImVec2(ImGui::GetWindowWidth() * 0.25f, ImGui::GetWindowWidth() * 0.25f);
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            
            draw_list->AddRectFilled(canvas_p, ImVec2(canvas_p.x + canvas_size.x, canvas_p.y + canvas_size.y), IM_COL32(20, 20, 20, 255));
            draw_list->AddRect(canvas_p, ImVec2(canvas_p.x + canvas_size.x, canvas_p.y + canvas_size.y), IM_COL32(255, 255, 255, 100));
            draw_list->AddLine(ImVec2(canvas_p.x + canvas_size.x/2, canvas_p.y), ImVec2(canvas_p.x + canvas_size.x/2, canvas_p.y + canvas_size.y), IM_COL32(255, 255, 255, 100));
            draw_list->AddLine(ImVec2(canvas_p.x, canvas_p.y + canvas_size.y/2), ImVec2(canvas_p.x + canvas_size.x, canvas_p.y + canvas_size.y/2), IM_COL32(255, 255, 255, 100));
            
            // For QAM64, step is 10% of canvas. Noise must not exceed 4% to prevent cluster overlap.
            float step = canvas_size.x / 10.0f;
            float max_noise = step * 0.35f;
            float noise_radius = ((100.0f - g_snr) / 100.0f) * max_noise;
            if (noise_radius < 2.0f) noise_radius = 2.0f;
            
            ImU32 point_col = IM_COL32(50, 150, 255, 255);
            if (g_snr < 50) point_col = IM_COL32(255, 255, 50, 255);
            if (g_snr < 30) point_col = IM_COL32(255, 50, 50, 255);
            
            float center_x = canvas_p.x + canvas_size.x / 2.0f;
            float center_y = canvas_p.y + canvas_size.y / 2.0f;
            for (int ix = -4; ix < 4; ix++) {
                for (int iy = -4; iy < 4; iy++) {
                    float cx = center_x + (ix + 0.5f) * step;
                    float cy = center_y + (iy + 0.5f) * step;
                    for (int p = 0; p < 5; p++) {
                        float rx = ((float)rand() / RAND_MAX - 0.5f) * noise_radius;
                        float ry = ((float)rand() / RAND_MAX - 0.5f) * noise_radius;
                        draw_list->AddCircleFilled(ImVec2(cx + rx, cy + ry), 1.5f, point_col);
                    }
                }
            }
            ImGui::Dummy(canvas_size);
            
            ImGui::Columns(1);
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            ImGui::Text("Live Lock Metrics");
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
        // TAB 6: CABLE FINDER
        // ==========================================
        if (ImGui::BeginTabItem("Cable Finder")) {
            ImGui::Spacing(); ImGui::Spacing();
            ImGui::Text("DVB-C Diagnostics");
            ImGui::Separator(); ImGui::Spacing();
            
            ImGui::Columns(2, "CableFinderCols", false);
            ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.45f);
            
            ImGui::PushItemWidth(ImGui::GetColumnWidth(0) * 0.5f);
            
            static int cab_freq = 410000;
            static int cab_sr = 6900;
            static int cab_qam = 2; // 0=16,1=32,2=64,3=128,4=256
            
            ImGui::InputInt("Frequency (kHz)", &cab_freq, 1000, 10000);
            ImGui::InputInt("Symbol Rate (KS/s)", &cab_sr, 100, 1000);
            ImGui::Combo("Modulation", &cab_qam, "QAM16\0QAM32\0QAM64\0QAM128\0QAM256\0");
            
            ImGui::PopItemWidth();
            
            ImGui::Spacing();
            if (ImGui::Button("Lock Tuner to Cable", ImVec2(300, 40))) {}
            
            ImGui::NextColumn();
            
            extern int g_snr, g_agc;
            
            // CONSTELLATION DIAGRAM
            ImGui::Spacing(); ImGui::Spacing();
            const char* qam_title = "Constellation Diagram\n(QAM64 Plot)";
            ImVec2 txt_size2 = ImGui::CalcTextSize(qam_title);
            float avail_txt2 = ImGui::GetContentRegionAvail().x;
            if (avail_txt2 > txt_size2.x) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail_txt2 - txt_size2.x) * 0.5f);
            ImGui::Text("%s", qam_title);
            ImVec2 canvas_p = ImGui::GetCursorScreenPos();
            float avail = ImGui::GetContentRegionAvail().x;
            float canvas_w = ImGui::GetWindowWidth() * 0.25f;
            if (avail > canvas_w) {
                canvas_p.x += (avail - canvas_w) * 0.5f;
                ImGui::SetCursorScreenPos(canvas_p);
            }
            ImVec2 canvas_size = ImVec2(ImGui::GetWindowWidth() * 0.25f, ImGui::GetWindowWidth() * 0.25f);
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            
            draw_list->AddRectFilled(canvas_p, ImVec2(canvas_p.x + canvas_size.x, canvas_p.y + canvas_size.y), IM_COL32(20, 20, 20, 255));
            draw_list->AddRect(canvas_p, ImVec2(canvas_p.x + canvas_size.x, canvas_p.y + canvas_size.y), IM_COL32(255, 255, 255, 100));
            draw_list->AddLine(ImVec2(canvas_p.x + canvas_size.x/2, canvas_p.y), ImVec2(canvas_p.x + canvas_size.x/2, canvas_p.y + canvas_size.y), IM_COL32(255, 255, 255, 100));
            draw_list->AddLine(ImVec2(canvas_p.x, canvas_p.y + canvas_size.y/2), ImVec2(canvas_p.x + canvas_size.x, canvas_p.y + canvas_size.y/2), IM_COL32(255, 255, 255, 100));
            
            // For QAM64, step is 10% of canvas. Noise must not exceed 4% to prevent cluster overlap.
            float step = canvas_size.x / 10.0f;
            float max_noise = step * 0.35f;
            float noise_radius = ((100.0f - g_snr) / 100.0f) * max_noise;
            if (noise_radius < 2.0f) noise_radius = 2.0f;
            
            ImU32 point_col = IM_COL32(255, 150, 50, 255);
            if (g_snr < 50) point_col = IM_COL32(255, 255, 50, 255);
            if (g_snr < 30) point_col = IM_COL32(255, 50, 50, 255);
            
            float center_x = canvas_p.x + canvas_size.x / 2.0f;
            float center_y = canvas_p.y + canvas_size.y / 2.0f;
            for (int ix = -4; ix < 4; ix++) {
                for (int iy = -4; iy < 4; iy++) {
                    float cx = center_x + (ix + 0.5f) * step;
                    float cy = center_y + (iy + 0.5f) * step;
                    for (int p = 0; p < 5; p++) {
                        float rx = ((float)rand() / RAND_MAX - 0.5f) * noise_radius;
                        float ry = ((float)rand() / RAND_MAX - 0.5f) * noise_radius;
                        draw_list->AddCircleFilled(ImVec2(cx + rx, cy + ry), 1.5f, point_col);
                    }
                }
            }
            ImGui::Dummy(canvas_size);
            
            ImGui::Columns(1);
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            ImGui::Text("Live Lock Metrics");
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
    ImGui::PopStyleVar();
}
