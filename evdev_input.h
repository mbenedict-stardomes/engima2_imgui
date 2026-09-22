#pragma once

// Initializes the evdev input system. 
// device_path is usually "/dev/input/event0" for the STB remote.
bool Evdev_Init(const char* device_path);

// Polls the input device and feeds events to ImGui IO
void Evdev_Poll();
bool Evdev_Poll_Plugin(); // Returns false if EXIT is pressed

// Closes the input device
void Evdev_Shutdown();
