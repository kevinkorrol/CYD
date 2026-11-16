# Grobot_Animations (vendored copy)

The eye animation library by Tanmay Wankar, MIT licensed (see `LICENSE`).

- **Origin:** https://github.com/tanmaywankar/Grobot_Animations, tag `v0.1.3`
  (commit `32833d0f2df4a8d9a791d9dee28f2fcf7dfed69b`, 28 Feb 2026).
- **Why it is copied here:** v0.1.4 removed the named moods (`setEmotion(String)`)
  and `moodSwitch()`, which this firmware was written against, and the firmware
  needs a forced blink. Keeping the copy in the repo also pins it.
- **What was copied:** `LICENSE`, `library.properties` (unchanged, still says
  0.1.3) and `src/`. The upstream README, examples, changelog and CI were left out.

## Changes from v0.1.3

1. `src/Grobot_Animations .cpp` renamed to `src/Grobot_Animations.cpp` (no space).
2. New public `GrobotEyes::blinkNow()`: backdates the blink timer so the next
   `renderEmotions()` frame starts a blink. `blink()` alone only fires every
   4–15 s.

Everything else is byte-for-byte v0.1.3. The firmware no longer calls
`moodSwitch()`; `src/FaceRenderer.cpp` does the cycling itself so it can report
each face. The cycle order and the IDLE1–3 eye values come from
`setEmotion(String)` in `src/Grobot_Animations.cpp`, not from `Moods.h`.
