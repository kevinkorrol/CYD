#include "FaceRenderer.h"
#include <Grobot_Animations.h>
#include "Config.h"
#include <esp_system.h>   // for esp_random()

static uint32_t _nextIdleSwitch = 0;
// ----- Sprite + Grobot state -----
static TFT_eSPI    *_tft     = nullptr;
static TFT_eSprite *_canvas  = nullptr;
static GrobotEyes  *_eyes    = nullptr;
static int16_t _lookX = 0, _lookY = 0;
static bool _hudOn = false;
static FaceShape _customL = {0, 0, 0, 30, 45};   // default = MOOD_NEUTRAL
static FaceShape _customR = {0, 0, 0, 30, 45};
static uint32_t _splashUntil = 0;             // 0 = inactive; else millis() at which to clear
static constexpr uint32_t SPLASH_DURATION_MS = 800;
static bool _paused = false;

// The named faces, in the eye library's cycling order (Grobot_Animations v0.1.3).
const char *const FACE_MOODS[FACE_MOOD_COUNT] = {
    "NEUTRAL", "HAPPY", "ANGRY", "SAD", "EXCITED",
    "ANNOYED", "QUESTIONING", "IDLE1", "IDLE2", "IDLE3"
};
static const char *FACE_CUSTOM = "CUSTOM";

bool isKnownMood(const char *name)
{
    for (auto m : FACE_MOODS) if (strcmp(name, m) == 0) return true;
    return false;
}

// The face on screen: a FACE_MOODS name or "CUSTOM". Separate from config.mood,
// the saved setting, which only the mood: command changes.
static char _shownMood[16] = "NEUTRAL";

// Cycling, done here rather than in the library's moodSwitch() so each face is
// reported. Same order, timing and random look as moodSwitch() in v0.1.3.
static int      _cycleIndex     = 0;
static uint32_t _lastCycleMs    = 0;
static uint32_t _cycleInterval  = 5000;

// Applies the shown face to the eyes without reporting it.
static void applyShownFace()
{
    if (!_eyes) return;
    if (strcmp(_shownMood, FACE_CUSTOM) == 0) {
        _eyes->setEmotion(MoodData{_customL.topH, _customL.botH, _customL.tilt, _customL.pR, _customL.radius},
                          MoodData{_customR.topH, _customR.botH, _customR.tilt, _customR.pR, _customR.radius});
    } else {
        _eyes->setEmotion(String(_shownMood));
    }
}

// Changes the face shown and reports it.
static void showFace(const char *name)
{
    strncpy(_shownMood, name, sizeof(_shownMood) - 1);
    _shownMood[sizeof(_shownMood) - 1] = '\0';
    applyShownFace();
    reportFace();
}

void reportFace() { Serial.printf("host_face:%s\n", _shownMood); }

// Internal helper — (re)builds the eyes with current colour state, keeping the
// face shown.
static void rebuildEyes()
{
    if (_eyes) { delete _eyes; _eyes = nullptr; }
    _eyes = new GrobotEyes(config.eyeColour, config.bgColour);
    applyShownFace();
}

void initFaceRenderer(TFT_eSPI *tft)
{
    _tft = tft;

    _canvas = new TFT_eSprite(_tft);
    if (!_canvas->createSprite(320, 140)) {     // was (320, 120)
        Serial.println("FaceRenderer: sprite alloc failed (heap fragmented?)");
        delete _canvas;
        _canvas = nullptr;
        return;
    }
    _canvas->fillSprite(config.bgColour);

    if (isKnownMood(config.mood)) {
        strncpy(_shownMood, config.mood, sizeof(_shownMood) - 1);
        _shownMood[sizeof(_shownMood) - 1] = '\0';
    }
    rebuildEyes();
    _hudOn = config.hudOn;
}

void showSplash()
{
    if (!_tft) return;
    _tft->setRotation(3); // re-orientation
    _tft->fillScreen(config.bgColour);
    _tft->setTextColor(config.eyeColour);
    _tft->setTextSize(3);
    int cx = _tft->width() / 2;
    int cy = _tft->height() / 2;
    _tft->setCursor(cx - 65, cy - 12);
    _tft->print("RSC-CYD");
    _splashUntil = millis() + SPLASH_DURATION_MS;
}

void serviceFaceRenderer()
{
    if (!_tft || !_canvas || !_eyes) return;
    if (_splashUntil != 0) {
        if (millis() < _splashUntil) return;     // still showing splash, skip render
        _tft->fillScreen(config.bgColour);       // splash done, clear before face takes over
        _splashUntil = 0;
    }
    // Nothing changes the face while it is paused (menu, pause, screen:off), so
    // nothing is reported for a face nobody can see.
    if (_paused) return;
    if (config.idleAnim && !config.moodAutoCycle) {
    uint32_t now = millis();
    if (_nextIdleSwitch == 0) {
        _nextIdleSwitch = now + 5000 + (esp_random() % 10000);
    } else if (now >= _nextIdleSwitch) {
        static const char* idles[] = {"IDLE1", "IDLE2", "IDLE3"};
        showFace(idles[esp_random() % 3]);   // shown only; the saved mood stays
        _nextIdleSwitch = now + 5000 + (esp_random() % 10000);
    }
    } else {
        _nextIdleSwitch = 0;
    }
    if (config.moodAutoCycle && millis() - _lastCycleMs > _cycleInterval) {
        _cycleIndex = (_cycleIndex + 1) % FACE_MOOD_COUNT;
        showFace(FACE_MOODS[_cycleIndex]);
        setLookAt(random(-30, 31), random(-20, 21));
        _lastCycleMs = millis();
        _cycleInterval = random(5000, 8000);
    }
    _eyes->renderEmotions(*_canvas);
    if (_hudOn) _eyes->HUD(*_tft);
    
}

void setEyeColour(uint16_t rgb565)
{
    if (config.eyeColour == rgb565) return;
    config.eyeColour = rgb565;
    if (_eyes) rebuildEyes();
}

void setBackgroundColour(uint16_t rgb565)
{
    if (config.bgColour == rgb565) return;
    config.bgColour = rgb565;
    if (_canvas) _canvas->fillSprite(config.bgColour);
    if (_eyes) rebuildEyes();
}

void drawTouchMarker(int x, int y)
{
    if (!_tft) return;
    _tft->fillCircle(x, y, 5, TFT_GREEN);
}

void setMood(const char *moodName)
{
    strncpy(config.mood, moodName, sizeof(config.mood) - 1);
    config.mood[sizeof(config.mood) - 1] = '\0';
    showFace(config.mood);
}

void setLookAt(int16_t x, int16_t y) { if (!_eyes) return; _lookX = x; _lookY = y; _eyes->lookAt(x, y); }
void getLookAt(int16_t &x, int16_t &y) { x = _lookX; y = _lookY; }
void triggerBlink() { if (_eyes) _eyes->blinkNow(); }
void setHud(bool on) { _hudOn = on; }

void setFaceCustom(const FaceShape &shape) {
    _customL = shape;
    _customR = shape;
    showFace(FACE_CUSTOM);
}
void setFaceLeft(const FaceShape &shape) {
    _customL = shape;
    showFace(FACE_CUSTOM);
}
void setFaceRight(const FaceShape &shape) {
    _customR = shape;
    showFace(FACE_CUSTOM);
}
void getFaceLeft (FaceShape &out) { out = _customL; }
void getFaceRight(FaceShape &out) { out = _customR; }
void pauseFace()    { _paused = true; }
void resumeFace()   { _paused = false; }
bool isFacePaused() { return _paused; }
void resetFaceState() { rebuildEyes(); }