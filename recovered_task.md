# Enigma2 ImGui UI Development Tasks

This document tracks the development progress of the core ImGui UI components for the Enigma2 plugin.

- `[x]` **1. Enigma2 Python/C++ Bridge Architecture**
  - `[x]` Hook ImGui rendering into a background thread via `ctypes`
  - `[x]` Implement Enigma2 `ActionMap` intercept to map remote control input to ImGui navigation
  - `[x]` Export Python callbacks via a thread-safe C++ queue (Action execution and Channel Playback)
  - `[/]` **Master UI Overlay Mode:** Maintain ImGui transparency over Live TV hardware video playback rather than exiting the plugin upon channel selection.

- `[x]` **2. Main Menu & Navigation**
  - `[x]` Design full-screen responsive layout (macOS style dock/tiles)
  - `[x]` Implement D-Pad optimized navigation (Up/Down/Left/Right/OK)

- `[ ]` **3. Setup & Configuration UIs**
  - `[ ]` **Tuner Scanning UIs:**
    - `[ ]` Automatic Scan (Full satellite/cable scan)
    - `[ ]` Manual Scan (Specific transponder frequency/symbol rate)
    - `[ ]` Blindscan (Scanning without predefined transponder data)
    - `[ ]` Signal Finder Menu (Live signal lock and strength meter for dish alignment)
  - `[ ]` **Network & Region Settings:** UI for IP config and localization.

- `[/]` **4. Live TV Channel List (Bouquets)**
  - `[x]` Live XML generation from Enigma2 `eServiceCenter` databases
  - `[x]` Fetch Favourites (`bouquets.tv`)
  - `[x]` Fetch Master A-Z List (Alphabetical sorted master list)
  - `[x]` Fetch Dynamic Terrestrial List (Namespace `0xEEEE0000` filtering for DVB-T/T2/ISDB-T)
  - `[ ]` Live TV Navigation: Pressing Up/Down/OK while watching Live TV should snap the Channel List back open over the video overlay.

- `[/]` **5. Multi-Tier Infobar & Telemetry (GlassHD Style)**
  - `[x]` **Small Infobar (Base):** Bottom-aligned minimal bar showing Channel Name and Time.
  - `[/]` **Small Infobar Auto-Hide:** Must automatically disappear after 2 seconds (configurable), returning the user to pure fullscreen Live TV.
  - `[/]` **Big Infobar (Expanded):** Triggered by pressing the 'INFO' button while the Small Infobar is visible.
  - `[ ]` **Big Infobar EPG:** Display current (NOW) and next (NEXT) program information.
  - `[ ]` **Big Infobar Telemetry:** Live updating SNR, AGC, and BER meters.
  - `[ ]` **Big Infobar Media Specs:** Displaying video resolution, encryption (FTA/Softcam), and audio tracks.
  - `[ ]` **Big Infobar Picons:** Rendering high-res channel logos from the Enigma2 Picon directory.
- [x] Fix Python backend SNR/AGC bug (Switch `getFrontendData` to `getFrontendStatus`).
- [x] Push SNR/AGC bug fix to STB and confirm.
- [ ] Refactor C++ `MENU_TUNER` into a dedicated `tuner_setup.cpp` file.
- [ ] Build ImGui TabBar system for Tuner Setup (Hardware, Scan, Signal Finders).
- [ ] Design **Hardware Configuration** Tab (LNB mapping, DVB Types).
- [ ] Design **Transponder Management** Tab (Add/Edit/Remove/Scan UI).
- [ ] Design **Satellite Signal Finder** Tab (incorporate existing IQ plotter).
- [ ] Design **Terrestrial Signal Finder** Tab.
- [ ] Design **Cable Signal Finder** Tab.
- [ ] Wire C++ Tuner UI inputs to Python IPC bridge.
- [ ] Build Python `nimmanager` hardware interrogation hooks.
- [ ] Build Python background `eScanner` trigger hooks.
