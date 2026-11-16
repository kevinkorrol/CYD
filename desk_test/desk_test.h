// The desk test's view into the stand-ins: the fake clock, serial port, NVS,
// LEDC, light sensor, touch, eye library and GUIslice element pool.
#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include "GUIslice.h"
#include "TouchHandler.h"

struct OutLine { uint32_t ms; std::string text; };

namespace desk {
    // Clock and randomness
    void     setMillis(uint32_t ms);
    void     seed(uint32_t s);

    // Serial
    void     feed(const std::string &bytes);           // bytes the "host" sends
    const std::vector<OutLine> &out();                 // every complete line the firmware printed
    size_t   mark();                                   // index of the next line to be printed

    // NVS
    void     nvsSet(const std::string &key, const std::string &value);
    bool     nvsHas(const std::string &key);
    std::string nvsGet(const std::string &key);
    int      nvsWrites();                              // number of put* calls

    // Hardware
    uint32_t pwmDuty();                                // last ledcWrite duty (0-255)
    void     setLdrRaw(uint16_t raw);                  // what analogRead returns (low = bright)

    // Touch
    void     queueTouch(TouchEventKind kind);          // delivered by the next pollTouchEvent()

    // Eye library
    std::string eyesEmotion();                         // last named setEmotion, or "CUSTOM"
    int      eyesBlinkNow();
    int      eyesRenders();
    int      eyesLookX();
    int      eyesLookY();

    // GUIslice
    gslc_tsElem *elem(int16_t id);                     // first element with this ID
    int16_t  sliderPos(int16_t id);
    void     dragSlider(int16_t id, int16_t pos);      // a finger move: SetPos from outside the firmware
    void     press(int16_t id);                        // a tap on a button: its touch callback, UP_IN
}

// From src/MenuCallbacks.cpp
bool CbBtnCommon(void *pvGui, void *pvElemRef, gslc_teTouch eTouch, int16_t nX, int16_t nY);
extern gslc_tsGui m_gui;
