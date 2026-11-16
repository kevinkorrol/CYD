// Implementations of the desk-test stand-ins, plus the firmware functions from
// files the test doesn't compile (TouchHandler.cpp, DebugOverlay.cpp,
// LED_Solution.cpp, src/icons/).
#include <Arduino.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <Grobot_Animations.h>
#include <esp_system.h>
#include <deque>
#include <map>
#include <stdexcept>
#include "GUIslice.h"
#include "TouchHandler.h"
#include "../desk_test.h"

// ---------------- clock, random ----------------
static uint32_t g_ms = 0;
static uint32_t g_rng = 12345;
static uint32_t nextRand() { g_rng = g_rng * 1103515245u + 12345u; return g_rng >> 1; }

uint32_t millis() { return g_ms; }
uint32_t micros() { return g_ms * 1000u; }
void delay(uint32_t ms) { g_ms += ms; }
long random(long max) { return max <= 0 ? 0 : (long)(nextRand() % (uint32_t)max); }
long random(long min, long max) { return max <= min ? min : min + random(max - min); }
uint32_t esp_random() { return nextRand(); }

void desk::setMillis(uint32_t ms) { g_ms = ms; }
void desk::seed(uint32_t s) { g_rng = s; }

// ---------------- Serial ----------------
HardwareSerial Serial;
static std::deque<char> g_in;
static std::string g_partial;
static std::vector<OutLine> g_out;

int HardwareSerial::available() { return (int)g_in.size(); }
int HardwareSerial::read() {
    if (g_in.empty()) return -1;
    char c = g_in.front(); g_in.pop_front(); return (unsigned char)c;
}
size_t HardwareSerial::write(char c) {
    if (c == '\n') { g_out.push_back({g_ms, g_partial}); g_partial.clear(); }
    else if (c != '\r') g_partial += c;
    return 1;
}
size_t HardwareSerial::print(const char *s) { size_t n = 0; for (; *s; s++) n += write(*s); return n; }
size_t HardwareSerial::println(const char *s) { size_t n = print(s); return n + write('\n'); }
size_t HardwareSerial::printf(const char *fmt, ...) {
    char buf[512];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    return print(buf);
}

void desk::feed(const std::string &bytes) { for (char c : bytes) g_in.push_back(c); }
const std::vector<OutLine> &desk::out() { return g_out; }
size_t desk::mark() { return g_out.size(); }

// ---------------- Preferences ----------------
static std::map<std::string, std::string> g_nvs;
static int g_nvsWrites = 0;
bool Preferences::begin(const char *, bool) { return true; }
bool Preferences::clear() { g_nvs.clear(); return true; }
size_t Preferences::putBool(const char *k, bool v) { g_nvs[k] = v ? "1" : "0"; g_nvsWrites++; return 1; }
size_t Preferences::putUChar(const char *k, uint8_t v) { g_nvs[k] = std::to_string(v); g_nvsWrites++; return 1; }
size_t Preferences::putUShort(const char *k, uint16_t v) { g_nvs[k] = std::to_string(v); g_nvsWrites++; return 2; }
size_t Preferences::putString(const char *k, const char *v) { g_nvs[k] = v; g_nvsWrites++; return strlen(v); }
bool Preferences::getBool(const char *k, bool def) { auto it = g_nvs.find(k); return it == g_nvs.end() ? def : it->second == "1"; }
uint8_t Preferences::getUChar(const char *k, uint8_t def) { auto it = g_nvs.find(k); return it == g_nvs.end() ? def : (uint8_t)atoi(it->second.c_str()); }
uint16_t Preferences::getUShort(const char *k, uint16_t def) { auto it = g_nvs.find(k); return it == g_nvs.end() ? def : (uint16_t)atoi(it->second.c_str()); }
String Preferences::getString(const char *k, const String &def) { auto it = g_nvs.find(k); return it == g_nvs.end() ? def : String(it->second); }

void desk::nvsSet(const std::string &k, const std::string &v) { g_nvs[k] = v; }
bool desk::nvsHas(const std::string &k) { return g_nvs.count(k) > 0; }
std::string desk::nvsGet(const std::string &k) { auto it = g_nvs.find(k); return it == g_nvs.end() ? "" : it->second; }
int desk::nvsWrites() { return g_nvsWrites; }

// ---------------- GPIO, ADC, LEDC, ESP ----------------
static uint32_t g_duty = 0;
static uint16_t g_ldr = 100;
void pinMode(int, int) {}
void digitalWrite(int, int) {}
uint16_t analogRead(int) { return g_ldr; }
void ledcSetup(int, int, int) {}
void ledcAttachPin(int, int) {}
void ledcWrite(int, uint32_t duty) { g_duty = duty; }
EspClass ESP;
void EspClass::restart() { throw std::runtime_error("ESP.restart() called"); }

uint32_t desk::pwmDuty() { return g_duty; }
void desk::setLdrRaw(uint16_t raw) { g_ldr = raw; }

// ---------------- TFT ----------------
void TFT_eSPI::fillScreen(uint32_t) {}

// ---------------- Eye library ----------------
static std::string g_emotion = "";
static int g_blinkNow = 0, g_renders = 0, g_lookX = 0, g_lookY = 0;
GrobotEyes::GrobotEyes(uint16_t, uint16_t) {}
void GrobotEyes::setEmotion(MoodData) { g_emotion = "CUSTOM"; }
void GrobotEyes::setEmotion(MoodData, MoodData) { g_emotion = "CUSTOM"; }
void GrobotEyes::setEmotion(String name) { g_emotion = name.str(); }
void GrobotEyes::renderEmotions(TFT_eSprite &) { g_renders++; }
void GrobotEyes::blink() {}
void GrobotEyes::blinkNow() { g_blinkNow++; }
void GrobotEyes::lookAt(int x, int y) { g_lookX = x; g_lookY = y; }
void GrobotEyes::moodSwitch(bool) { throw std::runtime_error("moodSwitch() must not be called"); }

std::string desk::eyesEmotion() { return g_emotion; }
int desk::eyesBlinkNow() { return g_blinkNow; }
int desk::eyesRenders() { return g_renders; }
int desk::eyesLookX() { return g_lookX; }
int desk::eyesLookY() { return g_lookY; }

// ---------------- GUIslice ----------------
static gslc_tsElem    g_elems[96];
static gslc_tsElemRef g_refs[96];
static int g_nElems = 0;

static gslc_tsElemRef *newElem(int16_t id, int16_t page) {
    if (g_nElems >= 96) throw std::runtime_error("GUIslice stand-in: element pool full");
    gslc_tsElem *e = &g_elems[g_nElems];
    *e = gslc_tsElem{};
    e->nId = id; e->nPage = page;
    g_refs[g_nElems].pElem = e;
    return &g_refs[g_nElems++];
}

bool gslc_Init(gslc_tsGui *pGui, gslc_tsDriver *, gslc_tsPage *, uint8_t, gslc_tsFont *, uint8_t) {
    pGui->nPageCur = 0; pGui->nPopup = -1; return true;
}
bool gslc_InitDebug(GSLC_CB_DEBUG_OUT) { return true; }
void gslc_InitTouchHandler(TouchHandler *) {}
bool gslc_FontSet(gslc_tsGui *, int16_t, gslc_teFontRefType, const void *, uint16_t) { return true; }
void gslc_PageAdd(gslc_tsGui *, int16_t, gslc_tsElem *, uint16_t, gslc_tsElemRef *, uint16_t) {}
void gslc_SetPageCur(gslc_tsGui *pGui, int16_t nPageId) { pGui->nPageCur = nPageId; }
void gslc_SetBkgndColor(gslc_tsGui *, gslc_tsColor) {}
bool gslc_GuiRotate(gslc_tsGui *, uint8_t) { return true; }
void gslc_PageRedrawSet(gslc_tsGui *, bool) {}
void gslc_Update(gslc_tsGui *) {}
void gslc_PopupShow(gslc_tsGui *pGui, int16_t nPageId, bool) { pGui->nPopup = nPageId; }
void gslc_PopupHide(gslc_tsGui *pGui) { pGui->nPopup = -1; }

gslc_tsImgRef gslc_GetImageFromProg(const unsigned char *pImgBuf, gslc_teImgRefFlags) { return gslc_tsImgRef{pImgBuf}; }
gslc_tsElemRef *gslc_PageFindElemById(gslc_tsGui *, int16_t nPageId, int16_t nElemId) {
    for (int i = 0; i < g_nElems; i++)
        if (g_elems[i].nId == nElemId && g_elems[i].nPage == nPageId) return &g_refs[i];
    return nullptr;
}
gslc_tsElem *gslc_GetElemFromRef(gslc_tsGui *, gslc_tsElemRef *r) { return r ? r->pElem : nullptr; }

gslc_tsElemRef *gslc_ElemCreateBox(gslc_tsGui *, int16_t id, int16_t pg, gslc_tsRect) { return newElem(id, pg); }
gslc_tsElemRef *gslc_ElemCreateLine(gslc_tsGui *, int16_t id, int16_t pg, int16_t, int16_t, int16_t, int16_t) { return newElem(id, pg); }
gslc_tsElemRef *gslc_ElemCreateTxt(gslc_tsGui *, int16_t id, int16_t pg, gslc_tsRect, char *s, uint8_t, int16_t) {
    gslc_tsElemRef *r = newElem(id, pg); r->pElem->txt = s; return r;
}
gslc_tsElemRef *gslc_ElemCreateBtnTxt(gslc_tsGui *, int16_t id, int16_t pg, gslc_tsRect, char *s, uint8_t, int16_t, GSLC_CB_TOUCH cb) {
    gslc_tsElemRef *r = newElem(id, pg); r->pElem->txt = s; r->pElem->pfuncTouch = cb; return r;
}
gslc_tsElemRef *gslc_ElemCreateBtnImg(gslc_tsGui *, int16_t id, int16_t pg, gslc_tsRect, gslc_tsImgRef img, gslc_tsImgRef, GSLC_CB_TOUCH cb) {
    gslc_tsElemRef *r = newElem(id, pg); r->pElem->img = img; r->pElem->pfuncTouch = cb; return r;
}
void gslc_ElemSetCol(gslc_tsGui *, gslc_tsElemRef *, gslc_tsColor, gslc_tsColor, gslc_tsColor) {}
void gslc_ElemSetTxtCol(gslc_tsGui *, gslc_tsElemRef *, gslc_tsColor) {}
void gslc_ElemSetTxtAlign(gslc_tsGui *, gslc_tsElemRef *, unsigned) {}
void gslc_ElemSetTxtStr(gslc_tsGui *, gslc_tsElemRef *, const char *) {}
void gslc_ElemSetRoundEn(gslc_tsGui *, gslc_tsElemRef *, bool) {}
void gslc_ElemSetFillEn(gslc_tsGui *, gslc_tsElemRef *, bool) {}
void gslc_ElemSetFrameEn(gslc_tsGui *, gslc_tsElemRef *, bool) {}
void gslc_ElemSetImage(gslc_tsGui *, gslc_tsElemRef *r, gslc_tsImgRef img, gslc_tsImgRef) { if (r) r->pElem->img = img; }

gslc_tsElemRef *gslc_ElemXSliderCreate(gslc_tsGui *, int16_t id, int16_t pg, gslc_tsXSlider *x, gslc_tsRect,
                                       int16_t mn, int16_t mx, int16_t pos, uint16_t, bool) {
    gslc_tsElemRef *r = newElem(id, pg);
    x->nPosMin = mn; x->nPosMax = mx; x->nPos = pos; x->pfuncXPos = nullptr;
    r->pElem->pXData = x;
    return r;
}
void gslc_ElemXSliderSetStyle(gslc_tsGui *, gslc_tsElemRef *, bool, gslc_tsColor, uint16_t, int16_t, gslc_tsColor) {}
void gslc_ElemXSliderSetPosFunc(gslc_tsGui *, gslc_tsElemRef *r, GSLC_CB_XSLIDER_POS cb) {
    ((gslc_tsXSlider *)r->pElem->pXData)->pfuncXPos = cb;
}
// As GUIslice's XSlider.c: clip, store, and call the callback synchronously if it changed.
void gslc_ElemXSliderSetPos(gslc_tsGui *pGui, gslc_tsElemRef *r, int16_t nPos) {
    if (!pGui || !r) return;
    gslc_tsXSlider *s = (gslc_tsXSlider *)r->pElem->pXData;
    if (nPos < s->nPosMin) nPos = s->nPosMin;
    else if (nPos > s->nPosMax) nPos = s->nPosMax;
    int16_t old = s->nPos;
    s->nPos = nPos;
    if (nPos != old && s->pfuncXPos) s->pfuncXPos((void *)pGui, (void *)r, nPos);
}

gslc_tsElemRef *gslc_ElemXTogglebtnCreate(gslc_tsGui *, int16_t id, int16_t pg, gslc_tsXTogglebtn *x, gslc_tsRect,
                                          gslc_tsColor, gslc_tsColor, gslc_tsColor, bool, bool on, GSLC_CB_TOUCH cb) {
    gslc_tsElemRef *r = newElem(id, pg); x->bOn = on; r->pElem->pXData = x; r->pElem->pfuncTouch = cb; return r;
}
bool gslc_ElemXTogglebtnGetState(gslc_tsGui *, gslc_tsElemRef *r) { return r && ((gslc_tsXTogglebtn *)r->pElem->pXData)->bOn; }

gslc_tsElem *desk::elem(int16_t id) {
    for (int i = 0; i < g_nElems; i++) if (g_elems[i].nId == id) return &g_elems[i];
    return nullptr;
}
static gslc_tsElemRef *refOf(int16_t id) {
    for (int i = 0; i < g_nElems; i++) if (g_elems[i].nId == id) return &g_refs[i];
    throw std::runtime_error("no element with id " + std::to_string(id));
}
int16_t desk::sliderPos(int16_t id) { return ((gslc_tsXSlider *)refOf(id)->pElem->pXData)->nPos; }
void desk::dragSlider(int16_t id, int16_t pos) { gslc_ElemXSliderSetPos(&m_gui, refOf(id), pos); }
void desk::press(int16_t id) {
    gslc_tsElemRef *r = refOf(id);
    if (r->pElem->pfuncTouch) r->pElem->pfuncTouch(&m_gui, r, GSLC_TOUCH_UP_IN, 0, 0);
}

// ---------------- Touch (stands in for src/TouchHandler.cpp) ----------------
static std::deque<TouchEventKind> g_touch;
void initTouch() {}
bool getTouchPoint(uint16_t &, uint16_t &, uint16_t &) { return false; }
bool pollTouchEvent(TouchEvent &ev) {
    ev.kind = TOUCH_NONE; ev.x = ev.y = 0;
    if (g_touch.empty()) return false;
    ev.kind = g_touch.front(); ev.x = 160; ev.y = 120; g_touch.pop_front();
    return true;
}
bool peekLastTouch(uint16_t &x, uint16_t &y, uint16_t &z) { x = y = z = 0; return false; }
void desk::queueTouch(TouchEventKind k) { g_touch.push_back(k); }

// ---------------- DebugOverlay.cpp, LED_Solution.cpp ----------------
void recordTouch(uint16_t, uint16_t, uint16_t) {}
void serviceDebugOverlay() {}
void printMemoryReport() {}
void printVersion() { Serial.println("Firmware: desk-test"); }
void printUptime() {}
void setLedColor(bool, bool, bool) {}
void setupLed() {}

// ---------------- src/icons (only the addresses matter) ----------------
#define ICON(n) extern "C" const unsigned short n[] = {0};
ICON(auto_bright_40x40px) ICON(back_40x40px) ICON(bright_40x40px) ICON(burger_icon_40x40px)
ICON(mic_on_icon_40x40px) ICON(mute_on_icon_40x40px) ICON(power_icon_40x40px) ICON(rsc_130x130)
ICON(volume_loud_icon2_40x40px) ICON(volume_low_icon2_40x40px) ICON(volume_mute_icon2_40x40px)
