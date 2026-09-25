# Advanced Tuner Setup & Transponder Management

This phase will build a comprehensive, multi-tab Hardware Configuration and Signal Acquisition suite within the ImGui plugin, replacing Enigma2's legacy tuner setup menus.

## Proposed Changes

### [Plugin Frontend (C++ ImGui)]

#### [MODIFY] main_menu.h / main_menu.cpp
- Expand the `MENU_TUNER` state into a full-screen window with a native `ImGui::BeginTabBar` containing the following interfaces:
  - **Hardware Configuration:** Select active tuner interfaces (Tuner A, Tuner B). Configure LNB routing (LNB1, LNB2), DiSEqC, and DVB Types (DVB-S/T/C).
  - **Transponder Management:** UI to Add, Remove, and Edit Transponders (Frequency, Polarity, Symbol Rate, FEC).
  - **Scan Modals:** Trigger Automatic Scan, Manual TP Scan, or Blindscan.
  - **Satellite Signal Finder:** Real-time SNR/AGC meters and Constellation IQ Plotter for DVB-S tuning.
  - **Terrestrial Signal Finder:** Real-time SNR/AGC meters specifically tailored for DVB-T frequencies.
  - **Cable Signal Finder:** Real-time SNR/AGC meters tailored for DVB-C.

#### [NEW] tuner_setup.cpp
- Extract the complex Tuner UI logic out of `main_menu.cpp` into a dedicated translation unit to manage the different Scan and LNB modes cleanly.

### [Plugin Backend (Python)]

#### [MODIFY] plugin.py
- Expose Enigma2 `nimmanager` (Network Interface Management) variables to C++ via IPC to detect how many physical tuners are on the STB and what their capabilities are.
- Build IPC hooks to trigger physical background scans (`eScanner`) and pipe scan progress/results back to the ImGui UI.

## Open Questions
- **DVB-C Testing:** Do you have an active DVB-C feed connected to the STB for testing the Cable Signal Finder, or should we strictly mock the UI for now?
- **Blindscan:** Does the STB tuner driver physically support hardware blindscan, or should we fallback to a brute-force software step-scan?

## Verification Plan
1. Validate `nimmanager` correctly detects the STB's hardware interfaces.
2. Ensure changing LNB or DVB types in the ImGui UI successfully reflects in Enigma2's core configuration files (`/etc/enigma2/settings`).
3. Verify the Signal Finders accurately display live transponder locks without causing video stutter on the active stream.
