// Desk-test stand-in for TFT_eSPI: draws nothing, counts screen fills.
#pragma once
#include <Arduino.h>

#define TFT_BLACK 0x0000
#define TFT_WHITE 0xFFFF
#define TFT_GREEN 0x07E0

class TFT_eSPI {
public:
    void init() {}
    void setRotation(uint8_t) {}
    void fillScreen(uint32_t);
    void setTextColor(uint16_t) {}
    void setTextColor(uint16_t, uint16_t) {}
    void setTextSize(uint8_t) {}
    void setCursor(int16_t, int16_t) {}
    int16_t width() const { return 320; }
    int16_t height() const { return 240; }
    void print(const char *) {}
    void print(int) {}
    void fillCircle(int32_t, int32_t, int32_t, uint32_t) {}
};

class TFT_eSprite : public TFT_eSPI {
public:
    explicit TFT_eSprite(TFT_eSPI *) {}
    bool createSprite(int16_t, int16_t) { return true; }
    void fillSprite(uint32_t) {}
};
