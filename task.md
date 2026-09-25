# Tasks: Tuner Management Wave

- [ ] Fix Python backend SNR/AGC bug (Switch `getFrontendData` to `getFrontendStatus`).
- [ ] Push SNR/AGC bug fix to STB and confirm.
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
