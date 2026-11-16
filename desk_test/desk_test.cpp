// Desk test: the firmware's serial behaviour, checked on a PC.
//
// src/main.cpp, Config.cpp, FaceRenderer.cpp, DisplayInit.cpp, LdrSensor.cpp,
// MenuCallbacks.cpp and UIManager.cpp are compiled unchanged against the
// stand-ins in stubs/. Each scenario runs in its own forked process, so each one
// starts from a fresh boot. A fake clock drives loop() in 10 ms steps.
#include <Arduino.h>
#include <sys/wait.h>
#include <unistd.h>
#include <functional>
#include <stdexcept>
#include "desk_test.h"
#include "gslc_ids.h"   // the Builder's element enum, extracted from include/test_GSLC.h by run.sh
#include "Config.h"

void setup();
void loop();

extern "C" const unsigned short auto_bright_40x40px[], bright_40x40px[], mic_on_icon_40x40px[],
    mute_on_icon_40x40px[], volume_loud_icon2_40x40px[], volume_low_icon2_40x40px[],
    volume_mute_icon2_40x40px[];

// ---------------------------------------------------------------- helpers
static int g_pass = 0, g_fail = 0, g_echo = 0;
static bool g_verbose = false;

static void check(const std::string &name, bool ok, const std::string &detail = "") {
    if (ok) { g_pass++; if (g_verbose) printf("    ok    %s\n", name.c_str()); }
    else    { g_fail++; printf("    FAIL  %s%s%s\n", name.c_str(), detail.empty() ? "" : " -- ", detail.c_str()); }
}

typedef std::vector<OutLine> Lines;
static Lines since(size_t m) { const Lines &o = desk::out(); return Lines(o.begin() + m, o.end()); }
static std::string show(const Lines &ls) {
    std::string s = "[";
    for (size_t i = 0; i < ls.size(); i++) s += (i ? " | " : "") + ls[i].text;
    return s + "]";
}
static bool starts(const std::string &s, const std::string &p) { return s.compare(0, p.size(), p) == 0; }
static int count(const Lines &ls, const std::string &prefix) {
    int n = 0; for (auto &l : ls) if (starts(l.text, prefix)) n++; return n;
}
static bool has(const Lines &ls, const std::string &text) {
    for (auto &l : ls) if (l.text == text) return true; return false;
}
static Lines only(const Lines &ls, const std::string &prefix) {
    Lines r; for (auto &l : ls) if (starts(l.text, prefix)) r.push_back(l); return r;
}
static bool exactly(const Lines &ls, std::vector<std::string> want) {
    if (ls.size() != want.size()) return false;
    for (size_t i = 0; i < want.size(); i++) if (ls[i].text != want[i]) return false;
    return true;
}

// The reply, optionally followed by host_face lines: a command that restarts
// the face (mood_cycle:on, screen:on) shows the overdue next face in the same loop().
static bool replyThenFaces(const Lines &ls, std::vector<std::string> want) {
    if (ls.size() < want.size()) return false;
    for (size_t i = 0; i < want.size(); i++) if (ls[i].text != want[i]) return false;
    for (size_t i = want.size(); i < ls.size(); i++) if (!starts(ls[i].text, "host_face:")) return false;
    return true;
}

static void step(uint32_t ms = 10) { desk::setMillis(millis() + ms); loop(); }
static void run(uint32_t ms) { for (uint32_t t = 0; t < ms; t += 10) step(10); }

// A host command: feed the line, run one loop(), return what came back. No
// command from the host may ever be answered with an audio request line.
static Lines send(const std::string &line) {
    size_t m = desk::mark();
    desk::feed(line + "\n");
    loop();
    Lines r = since(m);
    if (count(r, "host_vol") || count(r, "host_mute") || count(r, "host_mic")) {
        g_echo++;
        printf("    (audio request line in reply to '%s': %s)\n", line.c_str(), show(r).c_str());
    }
    return r;
}
static std::string get(const std::string &key) {
    Lines r = send(key + "?");
    return r.empty() ? "" : r.back().text;
}

static Lines boot() {
    desk::setMillis(0);
    size_t m = desk::mark();
    setup();
    return since(m);
}

static uint32_t duty(int pct) { return (uint32_t)(pct * 255) / 100; }
static const void *img(int16_t id) { gslc_tsElem *e = desk::elem(id); return e ? e->img.pImgBuf : nullptr; }
static int16_t volPos()    { return desk::sliderPos(E_ELEM_SLIDER2); }
static int16_t brightPos() { return desk::sliderPos(E_ELEM_SLIDER3); }

static const char *CYCLE[] = {"NEUTRAL", "HAPPY", "ANGRY", "SAD", "EXCITED",
                              "ANNOYED", "QUESTIONING", "IDLE1", "IDLE2", "IDLE3"};

// Throttled sends: at least 100 ms apart.
static bool spaced(const Lines &ls, uint32_t minGap) {
    for (size_t i = 1; i < ls.size(); i++) if (ls[i].ms - ls[i - 1].ms < minGap) return false;
    return true;
}

// ---------------------------------------------------------------- scenarios
static void boot_defaults() {
    Lines b = boot();
    size_t i = 0;
    while (i < b.size() && b[i].text != "host_ready") i++;
    check("boot sends host_ready", i < b.size(), show(b));
    Lines after(b.begin() + std::min(i + 1, b.size()), b.end());
    check("boot report follows host_ready", exactly(after, {"host_face:NEUTRAL", "host_cycle:on",
                                                             "host_bright:100", "host_auto_bright:off"}), show(after));
    check("boot sends no host_vol", count(b, "host_vol") == 0);
    check("boot sends one host_bright (the report)", count(b, "host_bright") == 1, show(b));
    check("boot volume slider at the shown volume (50)", volPos() == 50, std::to_string(volPos()));
    check("boot brightness slider at the setting (100)", brightPos() == 0, std::to_string(brightPos()));
    check("boot backlight at the setting", desk::pwmDuty() == duty(100));
    check("boot brightness icon is manual", img(E_ELEM_IMAGEBTN_BRIGHTNESS) == bright_40x40px);
    size_t m = desk::mark();
    run(6000);
    check("boot saves nothing", desk::nvsWrites() == 0, std::to_string(desk::nvsWrites()));
    check("no delayed host_vol after boot", count(since(m), "host_vol") == 0, show(since(m)));
    check("vol? answers vol:", get("vol") == "vol:            50");
}

static void boot_saved() {
    desk::nvsSet("bright", "40"); desk::nvsSet("mood", "SAD"); desk::nvsSet("moodCycle", "0");
    Lines b = boot();
    Lines rep = only(b, "host_");
    check("saved state in the boot report", exactly(rep, {"host_ready", "host_face:SAD", "host_cycle:off",
                                                         "host_bright:40", "host_auto_bright:off"}), show(rep));
    check("boot brightness slider at the saved 40", brightPos() == 60, std::to_string(brightPos()));
    check("boot backlight at the saved 40", desk::pwmDuty() == duty(40));
    size_t m = desk::mark();
    run(6000);
    check("boot sliders don't echo or save", only(since(m), "host_").empty() && desk::nvsWrites() == 0,
          show(since(m)) + " writes=" + std::to_string(desk::nvsWrites()));
}

static void host_audio() {
    desk::nvsSet("moodCycle", "0");
    boot();
    size_t m0 = desk::mark();
    check("vol:80 answers only OK", exactly(send("vol:80"), {"OK"}));
    check("vol:80 moves the slider", volPos() == 20, std::to_string(volPos()));
    check("vol? after vol:80", get("vol") == "vol:            80");
    check("vol:80 shows the loud icon", img(E_ELEM_IMAGEBTN_VOLUME) == volume_loud_icon2_40x40px);
    send("vol:30");
    check("vol:30 shows the low icon", img(E_ELEM_IMAGEBTN_VOLUME) == volume_low_icon2_40x40px);
    for (const char *bad : {"vol:-1", "vol:101", "vol:abc", "vol:"})
        check(std::string(bad) + " is refused", exactly(send(bad), {"ERR: 0-100"}));
    check("refused vol: leaves the volume", get("vol") == "vol:            30");
    check("vol:0 accepted", exactly(send("vol:0"), {"OK"}) && get("vol") == "vol:            0");
    send("vol:100");
    check("vol:100 puts the slider at the top", volPos() == 0);

    check("mute:on answers only OK", exactly(send("mute:on"), {"OK"}));
    check("mute? on", get("mute") == "mute:           on");
    check("mute:on shows the mute icon", img(E_ELEM_IMAGEBTN_VOLUME) == volume_mute_icon2_40x40px);
    send("mute:off");
    check("mute:off shows the level icon", img(E_ELEM_IMAGEBTN_VOLUME) == volume_loud_icon2_40x40px);
    check("mute:maybe is refused", exactly(send("mute:maybe"), {"ERR: bad bool"}));

    check("mic_mute:on answers only OK", exactly(send("mic_mute:on"), {"OK"}));
    check("mic_mute? on", get("mic_mute") == "mic_mute:       on");
    check("mic_mute:on shows the muted mic", img(E_ELEM_IMAGEBTN_MIC) == mute_on_icon_40x40px);
    send("mic_mute:off");
    check("mic_mute:off shows the live mic", img(E_ELEM_IMAGEBTN_MIC) == mic_on_icon_40x40px);

    run(6000);
    check("no delayed host_vol after vol:/mute:/mic_mute:", only(since(m0), "host_").empty(), show(only(since(m0), "host_")));
    check("host audio state is never saved", desk::nvsWrites() == 0 && !desk::nvsHas("vol") &&
          !desk::nvsHas("mute") && !desk::nvsHas("mic_mute"));

    size_t m = desk::mark();
    desk::press(E_ELEM_IMAGEBTN_VOLUME);
    check("panel mute press sends host_mute", exactly(since(m), {"host_mute"}), show(since(m)));
    check("panel mute press flips the icon at once", img(E_ELEM_IMAGEBTN_VOLUME) == volume_mute_icon2_40x40px);
    check("host's mute:off push corrects it, no echo", exactly(send("mute:off"), {"OK"}) &&
          img(E_ELEM_IMAGEBTN_VOLUME) == volume_loud_icon2_40x40px);
    m = desk::mark();
    desk::press(E_ELEM_IMAGEBTN_MIC);
    check("panel mic press sends host_mic", exactly(since(m), {"host_mic"}), show(since(m)));
}

static void slider_throttle() {
    desk::nvsSet("moodCycle", "0");
    boot();
    run(1000);
    size_t m = desk::mark();
    for (int p = 49; p >= 20; p--) { desk::dragSlider(E_ELEM_SLIDER2, p); step(20); }
    run(300);
    Lines v = only(since(m), "host_vol");
    check("volume drag sends several host_vol", v.size() >= 3, show(v));
    check("volume drag ends on the final value", !v.empty() && v.back().text == "host_vol:80", show(v));
    check("volume drag sends at most every 100 ms", spaced(v, 100), show(v));

    run(1000);
    m = desk::mark();
    desk::dragSlider(E_ELEM_SLIDER2, 18); step(10);
    desk::dragSlider(E_ELEM_SLIDER2, 16); step(10);
    desk::dragSlider(E_ELEM_SLIDER2, 15);           // let go 20 ms after the first send
    run(200);
    v = only(since(m), "host_vol");
    check("quick release still sends the last value", exactly(v, {"host_vol:82", "host_vol:85"}), show(v));

    m = desk::mark();
    send("vol:55");
    run(300);
    check("host vol: after a drag doesn't echo", count(since(m), "host_vol") == 0, show(since(m)));

    run(1000);
    m = desk::mark();
    for (int p = 5; p <= 50; p += 5) { desk::dragSlider(E_ELEM_SLIDER3, p); step(20); }
    run(300);
    Lines b = only(since(m), "host_bright");
    check("brightness drag ends on the final value", !b.empty() && b.back().text == "host_bright:50", show(b));
    check("brightness drag sends at most every 100 ms", spaced(b, 100), show(b));
    check("brightness drag with auto off sends no host_auto_bright", count(since(m), "host_auto_bright") == 0);
    check("brightness drag sets the backlight", desk::pwmDuty() == duty(50));
    run(6000);
    check("brightness drag is saved", desk::nvsGet("bright") == "50", desk::nvsGet("bright"));
}

static void moods() {
    desk::nvsSet("moodCycle", "0");
    boot();
    check("mood:happy reports then OK", exactly(send("mood:happy"), {"host_face:HAPPY", "OK"}));
    check("mood:happy shows HAPPY", desk::eyesEmotion() == "HAPPY");
    check("mood? HAPPY", get("mood") == "mood:           HAPPY");
    check("unknown mood refused, nothing reported", exactly(send("mood:SMILE"), {"ERR: unknown mood 'SMILE'"}));
    bool all = true;
    for (const char *mname : CYCLE) {
        Lines r = send(std::string("mood:") + mname);
        if (!has(r, std::string("host_face:") + mname)) all = false;
    }
    check("all 10 moods are accepted and reported", all);
    send("mood:HAPPY");
    check("face: reports CUSTOM", exactly(send("face:0,0,0,30,45"), {"host_face:CUSTOM", "OK"}));
    check("face: shows the custom shape", desk::eyesEmotion() == "CUSTOM");
    check("face: leaves the saved mood", get("mood") == "mood:           HAPPY");
    check("face_l: reports CUSTOM", has(send("face_l:0,30,0,30,45"), "host_face:CUSTOM"));
    check("face_r: reports CUSTOM", has(send("face_r:0,30,0,30,45"), "host_face:CUSTOM"));
    Lines bad = send("face:1,2");
    check("bad face: refused, nothing reported", bad.size() == 1 && starts(bad[0].text, "ERR:"), show(bad));
    int before = desk::eyesBlinkNow();
    check("blink answers OK", exactly(send("blink"), {"OK"}));
    check("blink calls blinkNow()", desk::eyesBlinkNow() == before + 1);
    run(6000);
    check("the saved mood is HAPPY", desk::nvsGet("mood") == "HAPPY", desk::nvsGet("mood"));
}

static void cycling() {
    boot();
    size_t m = desk::mark();
    run(62000);
    Lines f = only(since(m), "host_face:");
    check("cycling reports at least 8 faces in 62 s", f.size() >= 8, std::to_string(f.size()));
    bool order = true;
    for (size_t i = 0; i < f.size(); i++)
        if (f[i].text != std::string("host_face:") + CYCLE[(i + 1) % 10]) order = false;
    check("cycling goes in the library's order from HAPPY", order, show(f));
    check("first cycled face after 5 s", !f.empty() && f[0].ms > 5000 && f[0].ms <= 5010,
          f.empty() ? "" : std::to_string(f[0].ms));
    bool gaps = true;
    for (size_t i = 1; i < f.size(); i++) {
        uint32_t g = f[i].ms - f[i - 1].ms;
        if (g < 5000 || g > 8010) gaps = false;
    }
    check("cycling gaps are 5-8 s", gaps);
    check("cycling looks somewhere in range", abs(desk::eyesLookX()) <= 30 && abs(desk::eyesLookY()) <= 20);
    check("the eyes are drawn", desk::eyesRenders() > 1000);
    check("cycling doesn't change the saved mood", get("mood") == "mood:           NEUTRAL");
    check("cycling saves nothing", desk::nvsWrites() == 0);

    check("mood: while cycling reports it", has(send("mood:SAD"), "host_face:SAD"));
    m = desk::mark();
    run(20000);
    check("cycling carries on after mood:", count(since(m), "host_face:") >= 2);
    check("the saved mood stays SAD while cycling", get("mood") == "mood:           SAD");
    check("SAD is what NVS holds", desk::nvsGet("mood") == "SAD", desk::nvsGet("mood"));

    check("mood_cycle:off reports host_cycle:off", exactly(send("mood_cycle:off"), {"host_cycle:off", "OK"}));
    m = desk::mark();
    run(20000);
    check("no faces once cycling is off", count(since(m), "host_face:") == 0, show(since(m)));
    Lines on = send("mood_cycle:on");
    check("mood_cycle:on reports host_cycle:on", replyThenFaces(on, {"host_cycle:on", "OK"}), show(on));
}

static void idle_anim() {
    desk::nvsSet("moodCycle", "0"); desk::nvsSet("mood", "HAPPY");
    boot();
    send("idle_anim:on");
    size_t m = desk::mark();
    run(60000);
    Lines f = only(since(m), "host_face:");
    bool idle = !f.empty();
    for (auto &l : f) if (l.text != "host_face:IDLE1" && l.text != "host_face:IDLE2" && l.text != "host_face:IDLE3") idle = false;
    check("idle_anim reports only IDLE1-3", idle && f.size() >= 3, show(f));
    bool gaps = true;
    for (size_t i = 1; i < f.size(); i++) {
        uint32_t g = f[i].ms - f[i - 1].ms;
        if (g < 5000 || g > 15010) gaps = false;
    }
    check("idle_anim gaps are 5-15 s", gaps, show(f));
    check("idle_anim keeps the saved mood", get("mood") == "mood:           HAPPY");
    check("idle_anim doesn't save an idle face", desk::nvsGet("mood") == "HAPPY", desk::nvsGet("mood"));
}

static void brightness() {
    desk::nvsSet("moodCycle", "0");
    boot();
    check("bright:40 reports once, then OK", exactly(send("bright:40"), {"host_bright:40", "OK"}));
    check("bright:40 sets the backlight", desk::pwmDuty() == duty(40));
    check("bright:40 moves the slider", brightPos() == 60, std::to_string(brightPos()));
    check("bright? 40", get("bright") == "bright:         40%");
    check("bright:0 refused", exactly(send("bright:0"), {"ERR: 1-100"}));
    check("bright:101 refused", exactly(send("bright:101"), {"ERR: 1-100"}));

    check("auto_bright:on reports", exactly(send("auto_bright:on"), {"host_auto_bright:on", "OK: auto on"}));
    check("auto_bright:on shows the auto icon", img(E_ELEM_IMAGEBTN_BRIGHTNESS) == auto_bright_40x40px);
    desk::setLdrRaw(900);   // dark room
    size_t m = desk::mark();
    run(5000);
    check("auto-brightness dims the backlight", desk::pwmDuty() < duty(20), std::to_string(desk::pwmDuty()));
    check("auto-brightness reports nothing", only(since(m), "host_").empty(), show(since(m)));
    check("auto-brightness keeps the setting", get("bright") == "bright:         40%");
    run(6000);
    check("the saved brightness doesn't drift", desk::nvsGet("bright") == "40", desk::nvsGet("bright"));
    check("auto_bright:off reports", exactly(send("auto_bright:off"), {"host_auto_bright:off", "OK: auto off"}));
    check("auto off restores the setting", desk::pwmDuty() == duty(40), std::to_string(desk::pwmDuty()));
    check("auto off shows the manual icon", img(E_ELEM_IMAGEBTN_BRIGHTNESS) == bright_40x40px);

    send("auto_bright:on");
    run(2000);
    check("bright: with auto on reports both", exactly(send("bright:60"),
          {"host_bright:60", "host_auto_bright:off", "OK"}));
    check("bright: with auto on shows the manual icon", img(E_ELEM_IMAGEBTN_BRIGHTNESS) == bright_40x40px);
    check("bright:60 sets the backlight", desk::pwmDuty() == duty(60));

    m = desk::mark();
    desk::press(E_ELEM_IMAGEBTN_BRIGHTNESS);
    check("menu icon turns auto on", has(since(m), "host_auto_bright:on") &&
          img(E_ELEM_IMAGEBTN_BRIGHTNESS) == auto_bright_40x40px, show(since(m)));
    m = desk::mark();
    desk::press(E_ELEM_IMAGEBTN_BRIGHTNESS);
    check("menu icon turns auto off and restores the setting", has(since(m), "host_auto_bright:off") &&
          img(E_ELEM_IMAGEBTN_BRIGHTNESS) == bright_40x40px && desk::pwmDuty() == duty(60), show(since(m)));

    send("auto_bright:on");
    run(1000);
    m = desk::mark();
    for (int p = 35; p >= 10; p -= 5) { desk::dragSlider(E_ELEM_SLIDER3, p); step(20); }
    run(300);
    Lines s = since(m);
    check("slider with auto on turns it off once", count(s, "host_auto_bright:off") == 1, show(s));
    check("slider with auto on ends on its value", !only(s, "host_bright").empty() &&
          only(s, "host_bright").back().text == "host_bright:90", show(s));
    check("slider with auto on shows the manual icon", img(E_ELEM_IMAGEBTN_BRIGHTNESS) == bright_40x40px);

    check("bright_light:0 refused", exactly(send("bright_light:0"), {"ERR: 1-100"}));
    check("bright_light:1 accepted", exactly(send("bright_light:1"), {"OK"}));
    check("bright_light:100 accepted", exactly(send("bright_light:100"), {"OK"}));
    check("bright_light:101 refused", exactly(send("bright_light:101"), {"ERR: 1-100"}));
    check("bright_dark:0 refused", exactly(send("bright_dark:0"), {"ERR: 1-100"}));
    check("bright_dark:100 accepted", exactly(send("bright_dark:100"), {"OK"}));
}

static void screen() {
    desk::nvsSet("autoBright", "1");
    boot();
    check("mode:MENU while on", exactly(send("mode:MENU"), {"OK"}) && get("mode") == "mode:           MENU");
    check("screen:off answers OK", exactly(send("screen:off"), {"OK"}));
    check("screen:off turns the backlight off", desk::pwmDuty() == 0);
    check("screen:off closes the menu", get("mode") == "mode:           FACE");
    check("screen? off", get("screen") == "screen:         off");
    size_t m = desk::mark();
    desk::setLdrRaw(50);   // bright room: auto-brightness would turn the backlight up
    run(30000);
    check("stays dark with auto-brightness on", desk::pwmDuty() == 0, std::to_string(desk::pwmDuty()));
    check("no faces reported while dark", count(since(m), "host_face:") == 0, show(since(m)));
    check("mode:MENU refused while dark", exactly(send("mode:MENU"), {"ERR: screen is off"}) &&
          get("mode") == "mode:           FACE");
    desk::queueTouch(TOUCH_TAP); step();
    check("a tap while dark is ignored", get("screen") == "screen:         off");
    check("bright: while dark reports, stays dark", has(send("bright:30"), "host_bright:30") && desk::pwmDuty() == 0);
    Lines on = send("screen:on");
    check("screen:on answers OK", replyThenFaces(on, {"OK"}), show(on));
    check("screen:on restores the setting", desk::pwmDuty() == duty(30), std::to_string(desk::pwmDuty()));
    m = desk::mark();
    run(9000);
    check("the face resumes after screen:on", count(since(m), "host_face:") >= 1, show(since(m)));

    send("auto_bright:on");
    send("screen:off");
    run(3000);
    check("screen:off wins over auto-brightness again", desk::pwmDuty() == 0);
    m = desk::mark();
    desk::queueTouch(TOUCH_LONG_PRESS); step();
    check("long press while dark turns the screen on", get("screen") == "screen:         on");
    check("long press while dark sends nothing", only(since(m), "host_").empty() && !has(since(m), "OK: entered MENU"),
          show(since(m)));
    check("long press while dark doesn't open the menu", get("mode") == "mode:           FACE");
    check("long press while dark lights the backlight", desk::pwmDuty() > 0);
    m = desk::mark();
    run(9000);
    check("the face resumes after the long press", count(since(m), "host_face:") >= 1);
    m = desk::mark();
    desk::queueTouch(TOUCH_LONG_PRESS); step();
    check("long press while on still opens the menu", has(since(m), "OK: entered MENU") &&
          get("mode") == "mode:           MENU");
}

static void line_buffer() {
    desk::nvsSet("moodCycle", "0");
    boot();
    desk::feed("x");
    run(300);
    check("stray byte, 300 ms silence, then vol? answers vol:", exactly(send("vol?"), {"vol:            50"}));
    desk::feed("vo"); step(100);
    check("a pause under 200 ms keeps the partial line", exactly(send("l?"), {"vol:            50"}));
    desk::feed("mo");
    desk::setMillis(millis() + 500);   // loop() stalled; the rest arrived meanwhile
    check("a stalled loop doesn't drop a line", exactly(send("de?"), {"mode:           FACE"}));
}

static void host_startup() {
    boot();
    run(1000);
    send("");
    run(200);
    Lines r = send("vol?");
    check("start-up probe: vol? answers vol:", r.size() == 1 && starts(r[0].text, "vol:"), show(r));
    bool ok = true;
    for (const char *c : {"vol:40", "mute:off", "mic_mute:off", "screen:on", "resume"}) {
        Lines x = send(c);
        if (x.empty() || !starts(x.back().text, "OK")) ok = false;
    }
    check("start-up pushes are all accepted", ok);
    Lines rep = send("report");
    check("report: four lines then OK", exactly(rep, {"host_face:NEUTRAL", "host_cycle:on", "host_bright:100",
                                                     "host_auto_bright:off", "OK"}), show(rep));
}

// ---------------------------------------------------------------- driver
struct Scenario { const char *name; std::function<void()> fn; };

int main(int argc, char **argv) {
    std::string only = argc > 1 ? argv[1] : "";
    g_verbose = getenv("VERBOSE") != nullptr;
    Scenario all[] = {
        {"boot_defaults", boot_defaults}, {"boot_saved", boot_saved}, {"host_audio", host_audio},
        {"slider_throttle", slider_throttle}, {"moods", moods}, {"cycling", cycling},
        {"idle_anim", idle_anim}, {"brightness", brightness}, {"screen", screen},
        {"line_buffer", line_buffer}, {"host_startup", host_startup},
    };
    int pass = 0, fail = 0;
    for (auto &s : all) {
        if (!only.empty() && only != s.name) continue;
        int fd[2];
        if (pipe(fd) != 0) return 2;
        fflush(stdout);
        pid_t pid = fork();
        if (pid == 0) {
            close(fd[0]);
            printf("%s\n", s.name);
            desk::seed(0xC0FFEE);
            try { s.fn(); }
            catch (const std::exception &e) { check(std::string("no exception (") + e.what() + ")", false); }
            check("no audio request line in reply to any host command", g_echo == 0);
            fflush(stdout);
            int res[2] = {g_pass, g_fail};
            if (write(fd[1], res, sizeof res) != (ssize_t)sizeof res) _exit(3);
            _exit(0);
        }
        close(fd[1]);
        int res[2] = {0, 1};
        if (read(fd[0], res, sizeof res) != (ssize_t)sizeof res) printf("    FAIL  %s crashed\n", s.name);
        close(fd[0]);
        int st; waitpid(pid, &st, 0);
        printf("    %d passed, %d failed\n", res[0], res[1]);
        pass += res[0]; fail += res[1];
    }
    printf("\n%d checks: %d passed, %d failed\n", pass + fail, pass, fail);
    return fail ? 1 : 0;
}
