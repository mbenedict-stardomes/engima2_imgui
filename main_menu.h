#pragma once

enum MenuState {
    MENU_LIVETV = 0,
    MENU_HOME = 1,
    MENU_INFOBAR_SMALL = 2,
    MENU_INFOBAR_BIG = 3,
    MENU_TUNER = 4,
    MENU_CHANNELS = 5,
    MENU_NETWORK = 6,
    MENU_SYSTEM = 7
};

extern int g_currentState;


void MainMenu_Init();
bool MainMenu_Render();
