#ifndef DISPLAYINIT_H
#define DISPLAYINIT_H

#include <Arduino.h>

void initDisplay();
void setBacklight(uint8_t pct);    // 0-100 %, the setting: sets config.brightness and drives the PWM
void writeBacklight(uint8_t pct);  // 0-100 %, drives the PWM only (auto-brightness); 0 while the screen is off
void reattachBacklight();   // call after GUIslice re-inits TFT_eSPI
void setScreenOn(bool on);  // off = PWM 0, config.brightness kept; on = back to it
bool isScreenOn();
#endif