#pragma once
#include <TFT_eSPI.h>

void initFaceRenderer(TFT_eSPI *tft);
void showSplash();
void serviceFaceRenderer();   // call every loop()

// Runtime tuning hooks — destroys and rebuilds the eyes internally.
// Future serial-config layer will call these from UART command handlers.
void setEyeColour(uint16_t rgb565);
void setBackgroundColour(uint16_t rgb565);
void drawTouchMarker(int x, int y);
void setMood(const char *moodName);   // saves config.mood and shows it

// The named faces, in cycling order.
static constexpr int FACE_MOOD_COUNT = 10;
extern const char *const FACE_MOODS[FACE_MOOD_COUNT];
bool isKnownMood(const char *name);
void reportFace();   // prints host_face:<the face on screen, a name or CUSTOM>
void setLookAt(int16_t x, int16_t y);
void getLookAt(int16_t &x, int16_t &y);
void triggerBlink();   // blinks on the next frame
void setHud(bool on);

struct FaceShape { float topH, botH, tilt, pR, radius; };

void setFaceCustom(const FaceShape &shape);   // symmetric
void setFaceLeft  (const FaceShape &shape);   // left only, right preserved
void setFaceRight (const FaceShape &shape);   // right only, left preserved
void getFaceLeft  (FaceShape &out);
void getFaceRight (FaceShape &out);

void pauseFace();
void resumeFace();
bool isFacePaused();
void resetFaceState();