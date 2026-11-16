// Desk-test stand-in for the Arduino core: just enough of String, Serial,
// time, random, GPIO, LEDC and ESP for the firmware files under test.
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <string>

#define PROGMEM
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT 0

// ---- String (Arduino semantics where the firmware relies on them) ----
class String {
public:
    String() {}
    String(const char *s) : s_(s ? s : "") {}
    String(const std::string &s) : s_(s) {}
    explicit String(char c) : s_(1, c) {}
    String(int v)           : s_(std::to_string(v)) {}
    String(unsigned v)      : s_(std::to_string(v)) {}
    String(long v)          : s_(std::to_string(v)) {}
    String(unsigned long v) : s_(std::to_string(v)) {}
    String(unsigned char v) : s_(std::to_string((unsigned)v)) {}

    unsigned length() const { return (unsigned)s_.size(); }
    const char *c_str() const { return s_.c_str(); }
    long toInt() const { return atol(s_.c_str()); }   // Arduino: atol, "5x" -> 5, "x" -> 0

    void trim() {
        size_t a = s_.find_first_not_of(" \t\r\n");
        size_t b = s_.find_last_not_of(" \t\r\n");
        s_ = (a == std::string::npos) ? "" : s_.substr(a, b - a + 1);
    }
    void toLowerCase() { for (auto &c : s_) c = (char)tolower((unsigned char)c); }
    void toUpperCase() { for (auto &c : s_) c = (char)toupper((unsigned char)c); }
    bool endsWith(const char *suf) const {
        size_t n = strlen(suf);
        return s_.size() >= n && s_.compare(s_.size() - n, n, suf) == 0;
    }
    int indexOf(char c) const { size_t p = s_.find(c); return p == std::string::npos ? -1 : (int)p; }
    String substring(unsigned from) const { return from >= s_.size() ? String() : String(s_.substr(from)); }
    String substring(unsigned from, unsigned to) const {
        if (from > s_.size()) return String();
        if (to > s_.size()) to = (unsigned)s_.size();
        return to <= from ? String() : String(s_.substr(from, to - from));
    }

    String &operator=(const char *s) { s_ = s ? s : ""; return *this; }
    String &operator+=(char c) { s_ += c; return *this; }
    String &operator+=(const char *s) { s_ += s; return *this; }
    String &operator+=(const String &o) { s_ += o.s_; return *this; }
    friend String operator+(const String &a, const char *b) { return String(a.s_ + b); }
    friend String operator+(const String &a, const String &b) { return String(a.s_ + b.s_); }

    bool operator==(const char *o) const { return s_ == o; }
    bool operator==(const String &o) const { return s_ == o.s_; }
    bool operator!=(const char *o) const { return s_ != o; }
    bool operator!=(const String &o) const { return s_ != o.s_; }

    const std::string &str() const { return s_; }
private:
    std::string s_;
};

// ---- Serial: input from a test queue, output captured line by line ----
class HardwareSerial {
public:
    void begin(unsigned long) {}
    void setTimeout(unsigned long) {}
    int available();
    int read();
    size_t write(char c);
    size_t print(const char *s);
    size_t print(const String &s) { return print(s.c_str()); }
    size_t print(int v)  { return print(std::to_string(v).c_str()); }
    size_t println(const char *s = "");
    size_t println(const String &s) { return println(s.c_str()); }
    size_t println(int v) { return println(std::to_string(v).c_str()); }
    size_t printf(const char *fmt, ...) __attribute__((format(printf, 2, 3)));
};
extern HardwareSerial Serial;

// ---- Time and randomness ----
uint32_t millis();
uint32_t micros();
void delay(uint32_t ms);
long random(long max);
long random(long min, long max);

template <class T, class L, class H>
T constrain(T x, L lo, H hi) { return x < lo ? lo : (x > hi ? hi : x); }

// ---- GPIO, ADC, LEDC ----
void pinMode(int pin, int mode);
void digitalWrite(int pin, int v);
uint16_t analogRead(int pin);
void ledcSetup(int channel, int freq, int bits);
void ledcAttachPin(int pin, int channel);
void ledcWrite(int channel, uint32_t duty);

// ---- ESP ----
class EspClass {
public:
    uint32_t getFreeHeap() { return 200000; }
    uint32_t getMaxAllocHeap() { return 100000; }
    uint32_t getPsramSize() { return 0; }
    uint32_t getFreePsram() { return 0; }
    void restart();
};
extern EspClass ESP;
