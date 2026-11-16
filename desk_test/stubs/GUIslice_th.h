// Desk-test stand-in for GUIslice's touch-handler base class, which
// include/TouchHandler.h's CydTouchHandler derives from.
#pragma once
#include <stdint.h>

class THPoint {
public:
    THPoint() : x(0), y(0), z(0) {}
    THPoint(int16_t x_, int16_t y_, int16_t z_) : x(x_), y(y_), z(z_) {}
    int16_t x, y, z;
};

class TouchHandler {
public:
    virtual ~TouchHandler() {}
    virtual void begin() = 0;
    virtual THPoint getPoint() = 0;
};
