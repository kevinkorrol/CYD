// Desk-test stand-in for the eye library (lib/Grobot_Animations). Same public
// interface as our v0.1.3 copy; it records calls instead of drawing.
#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

struct MoodData {
    float topH, botH, tilt, pR, radius;
};

class GrobotEyes {
public:
    GrobotEyes(uint16_t eyeColor, uint16_t bgColor);
    void setEmotion(MoodData mood);
    void setEmotion(MoodData left, MoodData right);
    void setEmotion(String moodName);
    void renderEmotions(TFT_eSprite &canvas);
    void HUD(TFT_eSPI &canvas) {}
    void updateDeltaTime() {}
    void blink();
    void blinkNow();
    void lookAt(int x, int y);
    void moodSwitch(bool toSwitch);
};
