// Desk-test stand-in for ESP32 Preferences (NVS): an in-memory key/value map.
// The test reads and seeds it through desk_test.h.
#pragma once
#include <Arduino.h>

class Preferences {
public:
    bool begin(const char *ns, bool readOnly = false);
    void end() {}
    bool clear();
    size_t putBool(const char *k, bool v);
    size_t putUChar(const char *k, uint8_t v);
    size_t putUShort(const char *k, uint16_t v);
    size_t putString(const char *k, const char *v);
    size_t putString(const char *k, const String &v) { return putString(k, v.c_str()); }
    bool     getBool(const char *k, bool def = false);
    uint8_t  getUChar(const char *k, uint8_t def = 0);
    uint16_t getUShort(const char *k, uint16_t def = 0);
    String   getString(const char *k, const String &def = String());
};
