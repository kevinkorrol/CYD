#pragma once
#include <stdint.h>

enum UIMode : uint8_t { MODE_FACE, MODE_MENU };

void initUI();
void serviceUI();

// screen:on|off, also used by a long press while dark. Off closes the menu,
// pauses the face and writes PWM 0; on restores the saved brightness.
void setScreenPower(bool on);

UIMode getUIMode();
void   setUIMode(UIMode m);

// Kept for dispatch-table compatibility (Config.cpp's cmdMenu*)
void   menuBack();        // exits to FACE mode
bool   menuSelect(uint8_t idx);  // deprecated, no-op
void   printMode();
void   printMenuState();

void cmdMenuBack();   // calls menuBack() + prints OK
void cmdMenuState();  // calls printMenuState()