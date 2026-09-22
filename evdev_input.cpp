#include "evdev_input.h"
#include "imgui/imgui.h"
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

static int g_evdev_fd = -1;

bool Evdev_Init(const char* device_path) {
    g_evdev_fd = open(device_path, O_RDONLY | O_NONBLOCK);
    if (g_evdev_fd < 0) {
        printf("Failed to open %s: %d\n", device_path, errno);
        return false;
    }
    printf("Successfully opened evdev device: %s\n", device_path);
    return true;
}

ImGuiKey MapEvdevKeyToImGui(int evdev_key) {
    switch (evdev_key) {
        case KEY_UP: return ImGuiKey_UpArrow;
        case KEY_DOWN: return ImGuiKey_DownArrow;
        case KEY_LEFT: return ImGuiKey_LeftArrow;
        case KEY_RIGHT: return ImGuiKey_RightArrow;
        case KEY_ENTER: 
        case KEY_OK: return ImGuiKey_Enter;
        case KEY_ESC:
        case KEY_EXIT:
        case KEY_BACK: return ImGuiKey_Escape;
        case KEY_0: return ImGuiKey_0;
        case KEY_1: return ImGuiKey_1;
        case KEY_2: return ImGuiKey_2;
        case KEY_3: return ImGuiKey_3;
        case KEY_4: return ImGuiKey_4;
        case KEY_5: return ImGuiKey_5;
        case KEY_6: return ImGuiKey_6;
        case KEY_7: return ImGuiKey_7;
        case KEY_8: return ImGuiKey_8;
        case KEY_9: return ImGuiKey_9;
        case KEY_RED: return ImGuiKey_F1;
        case KEY_GREEN: return ImGuiKey_F2;
        case KEY_YELLOW: return ImGuiKey_F3;
        case KEY_BLUE: return ImGuiKey_F4;
        default: return ImGuiKey_None;
    }
}

void Evdev_Poll() {
    if (g_evdev_fd < 0) return;

    struct input_event ev;
    ImGuiIO& io = ImGui::GetIO();

    while (read(g_evdev_fd, &ev, sizeof(ev)) == sizeof(ev)) {
        if (ev.type == EV_KEY) {
            ImGuiKey imgui_key = MapEvdevKeyToImGui(ev.code);
            if (imgui_key != ImGuiKey_None) {
                // ev.value: 0 = release, 1 = press, 2 = repeat
                bool is_down = (ev.value == 1 || ev.value == 2);
                io.AddKeyEvent(imgui_key, is_down);
            }
            
            // Temporary exit hook for testing without crashing STB
            if (ev.code == KEY_POWER && ev.value == 1) {
                // Exit app if power button is pressed
                exit(0);
            }
        }
        
        // Future: If EV_REL or EV_ABS (mouse/gyro), we can call io.AddMousePosEvent()
    }
}

bool Evdev_Poll_Plugin() {
    if (g_evdev_fd < 0) return true;

    struct input_event ev;
    ImGuiIO& io = ImGui::GetIO();
    bool keep_running = true;

    while (read(g_evdev_fd, &ev, sizeof(ev)) == sizeof(ev)) {
        if (ev.type == EV_KEY) {
            ImGuiKey imgui_key = MapEvdevKeyToImGui(ev.code);
            if (imgui_key != ImGuiKey_None) {
                bool is_down = (ev.value == 1 || ev.value == 2);
                io.AddKeyEvent(imgui_key, is_down);
            }
            
            // Exit hook for plugin (Exit button or Power button)
            if ((ev.code == KEY_POWER || ev.code == KEY_EXIT || ev.code == KEY_ESC) && ev.value == 1) {
                keep_running = false;
            }
        }
    }
    return keep_running;
}

void Evdev_Shutdown() {
    if (g_evdev_fd >= 0) {
        close(g_evdev_fd);
        g_evdev_fd = -1;
    }
}
