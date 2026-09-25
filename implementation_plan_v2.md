# Master Implementation Plan: Enigma2 ImGui UI Replacement

The ultimate goal of this project is to completely replace the legacy Enigma2 user interface with a high-performance, native OpenGL (EGL) ImGui framework. This will allow the STB to eventually boot directly into the custom ImGui environment as a standalone UI option, bypassing the Enigma2 Python screens entirely while retaining full hardware functionality.

## Phase 1: Hardware Tuner Discovery (Completed)
- **Goal:** Dynamically detect physical demodulators on the motherboard and pass their capabilities to the C++ UI.
- **Status:** Complete. Python `nimmanager` successfully parses slots and passes the payload (e.g., `DVB-T2/C`) to C++ via IPC. Radio buttons render dynamically with correct spacing.

## Phase 2: Real Hardware Scanner Integration (Current)

We have successfully prototyped the frontend UX for scanning (the native ImGui overlay). The next step is the "real function build" where we replace the C++ UI simulation with actual hardware demodulator polling from Enigma2.

### User Review Required

Building a fully native scan dispatcher from scratch requires porting the logic from Enigma2's legacy `ScanSetup.py` (which is a massive 137KB state machine). We have two options for the backend architecture:

1. **Option A (Headless Wrapper):** We programmatically instantiate the native `Screens.ServiceScan` class in the background (hidden from the screen), feed it a constructed `scanList`, and intercept its `scanStatusChanged` events to feed back to our ImGui overlay via `SendImGuiAction("scan_progress|...")`.
2. **Option B (Direct eScanner API):** We bypass `ServiceScan` entirely, import `enigma.eScanner` directly, and build the entire `lamedb` saving and tuning loop ourselves in Python.

**Option A** is significantly safer for data integrity (guarantees `lamedb` is written correctly and tuner locks are respected). I recommend Option A.

### Open Questions

- Do you want to support Manual Transponder scanning immediately, or should we start by implementing just the **Automatic Scan (Full)** backend first to ensure the IPC data bridge works?
- During the scan, Enigma2 natively mutes audio and video. Should the ImGui framework do the same, or keep the background transparent?

### Proposed Changes

#### Python IPC Backend (`plugin_package/plugin.py`)
- Add a hidden headless dispatcher class that inherits from `ServiceScan`.
- Override the `updateProgress()` and `updateService()` methods.
- Instead of drawing to the Enigma2 screen, these methods will execute `self.send_action(f"scan_progress|{progress}|{service_name}")`.
- When `start_scan|0` is received via IPC, construct a `scanList` for the selected tuner and instantiate the headless dispatcher.

#### C++ ImGui Frontend (`tuner_ui.cpp` & `plugin.cpp`)
- Remove the C++ 60fps simulation timer.
- Modify `Enigma2_Action` in `plugin.cpp` to parse incoming `scan_progress|...` strings and update global variables.
- Bind the ImGui progress bar and service listbox directly to these global variables.

## Phase 3: High-Resolution Infobar Picons (Upcoming)
- **Goal:** Render high-res channel logos (Picons) directly from the Enigma2 Picon directory onto the glass HD Infobar.
- **Approach:** Pass the current Service Reference from Python to C++, construct the `1_0_1...png` filename, load the PNG directly into OpenGL texture memory via `stb_image.h`, and render it in the `infobar.cpp` overlay.

## Phase 4: WebSocket WebUI Integration (Upcoming)
- **Goal:** Add a background WebSocket server to allow external web app access for Remote Control, Telemetry, and EGL Screen Grabs.

