# Enigma2 Next-Gen GUI Architecture Exploration

This document explores the viability of several distinct platforms for achieving a robust, modern GUI environment for Enigma2 Set-Top Boxes (STBs) like Dreambox, VU+, and Octagon.

## Background Context
Traditional Enigma2 GUI development relies on a retained-mode Python framework where UI elements are defined in XML skin files and rendered via a software framebuffer (FBDev). Recently, Enigma2 distributions have been transitioning to an `e2egl` (EGL/GLES) backend to offload rendering to the STB's GPU. STB hardware (typically Broadcom or Hisilicon ARM/MIPS SoCs) often has strict constraints regarding CPU speed and available RAM (often 1GB-2GB).

## Platform Comparison

### Option A: ImGui-style C++/Python Interface
Using a framework like [Dear ImGui](https://github.com/ocornut/imgui) integrated via C++ or Python bindings.

**Pros:**
* **Extremely Lightweight:** Minimal memory footprint and CPU overhead, making it ideal for resource-constrained STBs.
* **Hardware Acceleration:** Natively supports OpenGL ES 2.0/3.0. It can hook directly into the STB's EGL context, fully utilizing the GPU.
* **High Performance:** Immediate mode GUIs update very quickly, allowing for smooth 60fps menus even on older SoCs.

**Cons:**
* **Paradigm Shift:** Immediate mode is a departure from Enigma2's traditional XML/event-driven skins.
* **Event Loop Conflict:** Enigma2 has its own main loop. We would either need to replace the Enigma2 GUI layer entirely or carefully synchronize the ImGui render loop.

### Option B: Rust GUI (Slint / Egui)
Using modern Rust-based embedded GUI frameworks. [Slint](https://slint.dev/) is declarative and optimized for embedded devices, while [egui](https://github.com/emilk/egui) is an immediate-mode library.

**Pros:**
* **Memory Safety & Speed:** Rust offers C++ level performance with memory safety.
* **Embedded Focus (Slint):** Slint is specifically designed for low-resource embedded Linux devices, consuming very little RAM while supporting hardware acceleration.
* **Modern Developer Experience:** Rust's tooling (Cargo) and Slint's declarative UI language make it a much more modern development environment than traditional Enigma2 C++.

**Cons:**
* **Integration Overhead:** We would need to write Rust bindings (via FFI or `cxx`) to interact with the underlying Enigma2 C++ core.
* **Learning Curve:** Requires the developer ecosystem to learn Rust (and Slint's DSL) instead of sticking to C++ or Python.

### Option C: WASM / Headless JS with SDL (`node-sdl`)
Instead of running a heavy embedded browser (WebKit), we use a headless JavaScript runtime (Node.js) or a WASM runtime combined with direct native rendering via SDL bindings (like [node-sdl](https://github.com/GroundUpCoder/node-sdl)).

**Pros:**
* **Web-Like Developer Ecosystem:** Developers can write application logic in JavaScript, TypeScript, or compile other languages to WASM.
* **Bypasses Browser Overhead:** By rendering directly to the framebuffer/EGL context via SDL, we avoid the massive memory and CPU tax of loading the DOM, WebKit, or Chromium.
* **High Interoperability:** Node.js/WASM has an excellent ecosystem for building local web servers or IPC bridges to talk to the Enigma2 core.

**Cons:**
* **No DOM/CSS:** Because there is no browser engine, developers cannot use standard HTML/React/CSS to layout the UI. They must draw shapes, textures, and text natively using the SDL canvas context.
* **Node.js Footprint:** While much lighter than a browser, the Node.js V8 engine still requires a decent chunk of memory (often 30MB-50MB minimum), which must be accounted for on older STBs.

## Proposed Approach (Recommendation)

If the goal is **maximum performance and minimal resource usage**, **Option A (ImGui)** or **Option B (Rust/Slint)** are the best choices. Slint in particular represents the modern "industry standard" for new embedded Linux HMIs.

If the goal is leveraging JS/WASM developers without the overhead of a full browser engine, **Option C (`node-sdl` approach)** is a very clever middle-ground. It allows for modern languages and WASM execution while rendering directly to the metal via SDL.

## User Review Required

> [!IMPORTANT]
> **Performance vs. Developer Experience**
> We need to decide whether the priority is eking out every ounce of performance natively (ImGui/Rust), or utilizing JS/WASM tooling by wrapping SDL.

## Open Questions

1. **Hardware Target:** Are we targeting legacy MIPS boxes with 512MB RAM, or only modern ARM 4K boxes (e.g., VU+ Duo 4K, Dreambox TWO)?
2. **Integration Level:** Are we building a *plugin* that runs inside the existing Enigma2 GUI, or a complete *replacement* for the Enigma2 UI layer?
3. **UI Paradigm:** If we go with the `node-sdl` approach, are you comfortable abandoning standard CSS/DOM layouts in favor of native canvas/SDL drawing?

---

## Wave 2: Advanced Tuner Setup & Transponder Management

This phase expands the ImGui plugin to fully support Hardware Configuration and Signal Acquisition, replacing Enigma2's legacy tuner setup menus.

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
