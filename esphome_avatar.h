// esphome_avatar.h
//
// Copyright (c) 2018 Shinya Ishikawa
// Copyright (c) 2026 Matthias Pfefferle
// SPDX-License-Identifier: MIT
//
// Header-only port of stack-chan/m5stack-avatar
// (https://github.com/stack-chan/m5stack-avatar), retargeted at ESPHome's
// display::Display API.
//
// Stays close to the original:
//   - geometry, blink timing, saccade gaze jumps, and breath cycle are
//     ported 1:1 from Face.cpp / Eye.cpp / Mouth.cpp / facialLoop
//   - expressions are carved into the EYE SHAPE (triangle lids for
//     angry/sad, crescent for happy, half-disc for sleepy) exactly like
//     the original Eye::draw() — this port has no eyebrows
//   - the mouth is the original wide rectangle (90px closed line that
//     narrows and opens vertically while speaking)
//   - top-right effect marks ported from Effect.h (heart, anger mark,
//     sweat drop, chill lines, sleep bubbles), pulsing with breath;
//     by default they follow the expression like the original
//
// Deviations:
//   - lip sync: the original samples real TTS audio levels; here
//     `set_speaking()` drives random syllable-like mouth levels instead
//   - `set_listening()`: attentive state the original doesn't have —
//     wide steady eyes, raised gaze, mouth stays closed
//   - Doubt: the original expressed doubt via eyebrows; here it gets
//     half-lidded flat-top eyes instead
//   - extra effects the original doesn't have: Music (eighth notes
//     drifting up the top-right corner) and Zzz ("ZZz" floating above
//     the head, replacing the bubbles as Sleepy's default mark)
//   - effect marks are animated instead of static: the notes dance side
//     to side in place, the "ZZz" writes itself out horizontally, heart
//     and bubbles float up with a horizontal wobble, the chill bars run
//     a wave; gloom lines hang over the brow and the anger vein throbs
//     at the temple — anchored on the face, where those marks belong
//   - idle mode (`set_idle()`): a self-running show neither the original
//     lib nor the stack-chan firmware has — wanders between calm moods,
//     dozes off after inactivity (Sleepy + "ZZz" + slow deep breathing),
//     and wakes up on its own or when it has to speak or listen
//
// Default face geometry assumes a 320x240 panel (M5Stack Core / CoreS3).
// For smaller displays, scale via FaceGeometry or wrap with your own scale.
//
// Drop this into your ESPHome config dir and add to esphome.yaml:
//   esphome:
//     includes:
//       - esphome_avatar.h
//
// Then in a `display:` lambda, instantiate a static Avatar and call draw(it).

#pragma once

#include "esphome/core/color.h"
#include "esphome/components/display/display.h"
#include <cmath>
#include <algorithm>

namespace stackchan_avatar {

enum class Expression : uint8_t {
  Neutral = 0,
  Happy   = 1,
  Sad     = 2,
  Angry   = 3,
  Sleepy  = 4,
  Doubt   = 5,
};

// Top-right effect marks. Auto follows the expression like the original
// Effect.h (Happy → Heart, Angry → Anger, Sad → Chill, Doubt → Sweat,
// Sleepy → Zzz); the rest force a specific mark, None hides them.
enum class Effect : uint8_t {
  Auto    = 0,
  None    = 1,
  Music   = 2,  // eighth notes drifting upward (e.g. media player active)
  Zzz     = 3,  // "ZZz" floating above the head
  Heart   = 4,
  Anger   = 5,
  Sweat   = 6,
  Chill   = 7,
  Bubbles = 8,  // the original's Sleepy mark
};

// Three-slot palette mirroring the original m5stack-avatar ColorPalette:
//   background — fills the screen each frame (also used to carve eye lids)
//   primary    — eyes and mouth fill
//   secondary  — inner-mouth accent while the mouth is open
struct ColorPalette {
  esphome::Color background{0,   0,   0};
  esphome::Color primary   {255, 255, 255};
  esphome::Color secondary {255, 255, 255};
};

// 320x240 face layout, ported from the Face() default constructor.
// Original uses BoundingRect(top, left) per part where top/left is the
// part's CENTER — these are the same values as (x, y).
struct FaceGeometry {
  // Eyes (viewer's left = character's right = smaller x)
  uint16_t left_eye_x  = 90;
  uint16_t left_eye_y  = 93;
  uint16_t right_eye_x = 230;
  uint16_t right_eye_y = 96;
  uint16_t eye_radius  = 8;

  // Mouth — original Mouth(50, 90, 4, 60): wide thin line when closed,
  // narrows and opens vertically while speaking.
  uint16_t mouth_x     = 163;
  uint16_t mouth_y     = 148;
  uint16_t mouth_min_w = 50;
  uint16_t mouth_max_w = 90;
  uint16_t mouth_min_h = 4;
  uint16_t mouth_max_h = 60;
};

class Avatar {
 public:
  // --- state setters, all designed to be called from interval/lambda ---
  void set_expression(Expression e) { expression_ = e; }
  void set_speaking(bool s)         { speaking_ = s; }
  // Listening: attentive face — wide steady eyes looking slightly up,
  // saccades paused, mouth firmly closed. No mouth animation.
  void set_listening(bool l)        { listening_ = l; }
  void set_effect(Effect e)         { effect_ = e; }

  // Idle mode: a self-running little show — wanders between calm moods
  // (mostly neutral, sometimes happy or doubt), dozes off after a while
  // of inactivity (Sleepy face, "ZZz", slow deep breathing, lowered
  // gaze), and wakes up on its own or as soon as it has to speak or
  // listen. While enabled it owns the expression; set_expression()
  // calls are overridden each frame.
  void set_idle(bool i)                     { idle_ = i; }
  // Doze after this much inactivity (default 5 min; 0 = never doze)
  void set_idle_sleep_after(uint32_t ms)    { idle_sleep_after_ = ms; }
  // Nap length before waking up on its own (default 2 min)
  void set_idle_sleep_duration(uint32_t ms) { idle_sleep_duration_ = ms; }

  // Manual animation axes (skip update_animations if you drive these):
  void set_blink_phase(float p)     { blink_ = std::clamp(p, 0.0f, 1.0f); }
  void set_breath_phase(float p)    { breath_ = std::clamp(p, -1.0f, 1.0f); }
  void set_speak_phase(float p)     { speak_ = std::clamp(p, 0.0f, 1.0f); }
  void set_gaze(float v, float h) {
    gaze_v_ = std::clamp(v, -1.0f, 1.0f);
    gaze_h_ = std::clamp(h, -1.0f, 1.0f);
  }

  // --- palette control ---
  // Pass a full palette in one call.
  void set_palette(const ColorPalette &p) { palette_ = p; }
  // Or set individual slots — handy from HA service calls / globals.
  void set_background(esphome::Color c) { palette_.background = c; }
  void set_primary(esphome::Color c)    { palette_.primary    = c; }
  void set_secondary(esphome::Color c)  { palette_.secondary  = c; }
  // Convenience for setting from packed 0xRRGGBB ints (typical HA service
  // payload). Bits: 0xRRGGBB.
  void set_background_rgb(uint32_t rgb) { palette_.background = unpack_(rgb); }
  void set_primary_rgb(uint32_t rgb)    { palette_.primary    = unpack_(rgb); }
  void set_secondary_rgb(uint32_t rgb)  { palette_.secondary  = unpack_(rgb); }

  void set_geometry(const FaceGeometry &g) { geom_ = g; }

  // Drive blink, saccades, breath, and speech from a single millis()
  // value. Call once per frame before draw(). Timings ported from the
  // original facialLoop. If you want manual control over any axis, call
  // the set_*_phase setters yourself instead.
  void update_animations(uint32_t now_ms) {
    // --- idle mode: mood wander -> doze -> wake, on top of everything else
    if (!idle_) {
      last_activity_ms_ = now_ms;  // timer starts fresh when idle is enabled
      idle_state_ = IdleState::Awake;
    } else {
      if (speaking_ || listening_) last_activity_ms_ = now_ms;
      switch (idle_state_) {
        case IdleState::Awake:
          if (idle_sleep_after_ > 0 &&
              now_ms - last_activity_ms_ >= idle_sleep_after_) {
            idle_state_ = IdleState::Sleeping;
            idle_until_ms_ = now_ms + idle_sleep_duration_;
          } else if ((int32_t)(now_ms - mood_until_ms_) >= 0) {
            // calm mood wander, held for 15-45 s each
            const float roll = frand_();
            expression_ = roll < 0.5f   ? Expression::Neutral
                          : roll < 0.8f ? Expression::Happy
                                        : Expression::Doubt;
            mood_until_ms_ = now_ms + 15000 + (uint32_t)(frand_() * 30000.0f);
          }
          break;
        case IdleState::Sleeping:
          expression_ = Expression::Sleepy;
          if (speaking_ || listening_ ||
              (int32_t)(now_ms - idle_until_ms_) >= 0) {
            idle_state_ = IdleState::Waking;
            idle_until_ms_ = now_ms + 3000;
            last_activity_ms_ = now_ms;
          }
          break;
        case IdleState::Waking:
          expression_ = Expression::Happy;  // bright-eyed stretch
          if ((int32_t)(now_ms - idle_until_ms_) >= 0) {
            idle_state_ = IdleState::Awake;
            expression_ = Expression::Neutral;
            mood_until_ms_ = now_ms + 15000 + (uint32_t)(frand_() * 30000.0f);
          }
          break;
      }
    }
    const bool asleep = idle_ && idle_state_ == IdleState::Sleeping;

    // --- breath: ~3.3 s sine (original: 100 ticks at 33 ms), slowing to
    // a deep ~6.6 s cycle while dozing. Phase-accumulated so the rate
    // change doesn't make the face jump.
    if (last_ms_ == 0) last_ms_ = now_ms;
    breath_phase_ +=
        (float)(now_ms - last_ms_) / 1000.0f / (asleep ? 6.6f : 3.3f);
    breath_phase_ -= (int)breath_phase_;
    breath_ = std::sin(breath_phase_ * 2.0f * (float)M_PI);
    last_ms_ = now_ms;

    // --- saccades: gaze jumps to a random direction, then holds.
    // Original: interval 500 + 100*random(20) ms, gaze in [-1, 1].
    // While listening, the gaze locks on slightly raised — focused on
    // the person talking instead of wandering. While dozing it settles
    // low instead.
    if (listening_) {
      gaze_h_ = 0.0f;
      gaze_v_ = -0.4f;
    } else if (asleep) {
      gaze_h_ = 0.0f;
      gaze_v_ = 0.6f;
    } else if ((int32_t)(now_ms - next_saccade_ms_) >= 0) {
      gaze_h_ = frand_() * 2.0f - 1.0f;
      gaze_v_ = frand_() * 2.0f - 1.0f;
      next_saccade_ms_ = now_ms + 500 + (uint32_t)(frand_() * 2000.0f);
    }

    // --- blink: binary open/close like the original.
    // Open 2500 + 100*random(20) ms, closed 300 + 10*random(20) ms.
    if ((int32_t)(now_ms - next_blink_ms_) >= 0) {
      eye_open_ = !eye_open_;
      next_blink_ms_ = now_ms + (eye_open_
                                     ? 2500 + (uint32_t)(frand_() * 2000.0f)
                                     : 300 + (uint32_t)(frand_() * 200.0f));
    }
    blink_ = eye_open_ ? 1.0f : 0.0f;

    // --- effects: shared 3 s loop phase for drifting/bobbing marks
    effect_phase_ = (float)(now_ms % 3000) / 3000.0f;

    // --- speech: random mouth levels at syllable rate, eased — stands in
    // for the original lipSync task, which samples real TTS audio levels
    // every 33 ms.
    if (speaking_) {
      if ((int32_t)(now_ms - next_syllable_ms_) >= 0) {
        speak_target_ = frand_();
        next_syllable_ms_ = now_ms + 80 + (uint32_t)(frand_() * 180.0f);
      }
      speak_ += (speak_target_ - speak_) * 0.5f;
    } else {
      // Decay quickly when speech ends so the mouth closes smoothly.
      speak_target_ = 0.0f;
      speak_ *= 0.6f;
      if (speak_ < 0.02f) speak_ = 0.0f;
    }
  }

  // --- the only call you need from your display lambda ---
  void draw(esphome::display::Display &it) {
    it.fill(palette_.background);
    const int by = (int)(breath_ * 3.0f);  // original: all parts shift by breath*3
    const int gx = (int)(gaze_h_ * 3.0f);  // original: gaze offsets eyes by *3
    const int gy = (int)(gaze_v_ * 3.0f);
    const bool open = blink_ >= 0.5f;

    // is_left refers to the CHARACTER's left (viewer's right), matching
    // the original Eye(r, isLeft) constructor args.
    draw_eye_(it, geom_.left_eye_x + gx,  geom_.left_eye_y + by + gy,
              /*is_left=*/false, open);
    draw_eye_(it, geom_.right_eye_x + gx, geom_.right_eye_y + by + gy,
              /*is_left=*/true, open);

    draw_mouth_(it, by);
    draw_effect_(it);
  }

 protected:
  // Port of the original Eye::draw(): filled circle, then the expression
  // is carved out of it with background-colored shapes.
  void draw_eye_(esphome::display::Display &it, int cx, int cy,
                 bool is_left, bool open) {
    int r = geom_.eye_radius;
    if (listening_) r += std::max(1, r / 4);  // wide, attentive eyes

    if (!open) {
      // Blink: flat horizontal bar (original: fillRect 2r x 4)
      it.filled_rectangle(cx - r, cy - 2, r * 2, 4, palette_.primary);
      return;
    }

    it.filled_circle(cx, cy, r, palette_.primary);

    switch (expression_) {
      case Expression::Angry:
      case Expression::Sad: {
        // Slanted lid: background triangle over the top half. The low
        // corner sits inward for angry, outward for sad (original:
        // x2 = !isLeft != !(exp == Sad) ? x0 : x1).
        const bool sad = expression_ == Expression::Sad;
        const int x0 = cx - r, y0 = cy - r, x1 = cx + r;
        const int x2 = (is_left != sad) ? x0 : x1;
        fill_triangle_(it, x0, y0, x1, y0, x2, y0 + r, palette_.background);
        break;
      }
      case Expression::Happy:
        // Upper crescent ("^_^"): carve center circle + bottom half.
        it.filled_circle(cx, cy, (int)(r / 1.5f), palette_.background);
        it.filled_rectangle(cx - r - 2, cy, r * 2 + 4, r + 2,
                            palette_.background);
        break;
      case Expression::Sleepy:
        // Droopy lower half-disc: carve the top half.
        it.filled_rectangle(cx - r - 2, cy - r, r * 2 + 4, r + 2,
                            palette_.background);
        break;
      case Expression::Doubt:
        // Half-lidded flat top (the original used eyebrows for doubt).
        it.filled_rectangle(cx - r - 2, cy - r, r * 2 + 4, (r * 3) / 5,
                            palette_.background);
        break;
      default:
        break;
    }
  }

  // Filled triangle via scanlines — ESPHome's Display::filled_triangle()
  // is recent; this keeps the minimum supported version low.
  void fill_triangle_(esphome::display::Display &it, int x0, int y0, int x1,
                      int y1, int x2, int y2, esphome::Color c) {
    if (y0 > y1) { std::swap(x0, x1); std::swap(y0, y1); }
    if (y0 > y2) { std::swap(x0, x2); std::swap(y0, y2); }
    if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); }
    if (y0 == y2) {  // degenerate: all on one row
      const int xl = std::min(x0, std::min(x1, x2));
      const int xr = std::max(x0, std::max(x1, x2));
      it.horizontal_line(xl, y0, xr - xl + 1, c);
      return;
    }
    for (int y = y0; y <= y2; y++) {
      // long edge (v0 -> v2) and the short edge active on this row
      const float xa = x0 + (float)(x2 - x0) * (float)(y - y0) / (float)(y2 - y0);
      float xb;
      if (y < y1) {
        xb = x0 + (float)(x1 - x0) * (float)(y - y0) / (float)(y1 - y0);
      } else if (y2 > y1) {
        xb = x1 + (float)(x2 - x1) * (float)(y - y1) / (float)(y2 - y1);
      } else {
        xb = (float)x1;
      }
      const int xl = (int)std::floor(std::min(xa, xb));
      const int xr = (int)std::ceil(std::max(xa, xb));
      it.horizontal_line(xl, y, xr - xl + 1, c);
    }
  }

  // Port of the original Mouth::draw(): a filled rectangle that is wide
  // and thin when closed (90x4) and narrows while opening (50x60).
  void draw_mouth_(esphome::display::Display &it, int by) {
    const float open = std::clamp(speak_, 0.0f, 1.0f);
    const int h = geom_.mouth_min_h +
                  (int)((geom_.mouth_max_h - geom_.mouth_min_h) * open);
    const int w = geom_.mouth_max_w -
                  (int)((geom_.mouth_max_w - geom_.mouth_min_w) * open);
    const int x = geom_.mouth_x - w / 2;
    // Original: mouth shifts by breath*3 (face) plus breath*2 (mouth-local).
    const int y = geom_.mouth_y - h / 2 + by + (int)(breath_ * 2.0f);

    it.filled_rectangle(x, y, w, h, palette_.primary);
    // Inner-mouth accent — invisible unless secondary differs from primary.
    if (h > 12 && w > 12) {
      it.filled_rectangle(x + 3, y + 3, w - 6, h - 6, palette_.secondary);
    }
  }

  // --- effects: marks ported from the original Effect.h plus the
  // Music / Zzz additions. Motion is mostly horizontal (dancing notes,
  // text-like "ZZz", wobbling floaters); sweat drop, anger vein, and
  // gloom lines sit anchored on the face, where those marks belong.
  void draw_effect_(esphome::display::Display &it) {
    Effect e = effect_;
    if (e == Effect::Auto) {
      switch (expression_) {
        case Expression::Happy:  e = Effect::Heart;  break;
        case Expression::Angry:  e = Effect::Anger;  break;
        case Expression::Sad:    e = Effect::Chill;  break;
        case Expression::Doubt:  e = Effect::Sweat;  break;
        case Expression::Sleepy: e = Effect::Zzz;    break;
        default:                 e = Effect::None;   break;
      }
    }
    switch (e) {
      case Effect::Music:
        draw_music_(it);
        break;
      case Effect::Zzz:
        draw_zzz_(it);
        break;
      case Effect::Heart: {
        // One heart floating up, growing, with a soft horizontal wobble.
        const float p = effect_phase_;
        if (p <= 0.85f) {
          const int x = 284 + (int)(std::sin(p * 4.0f * (float)M_PI) * 4.0f);
          const int y = 80 - (int)(p * 52.0f);
          draw_heart_(it, x, y, 6 + (int)(p * 5.0f));
        }
        break;
      }
      case Effect::Anger:
        // The anger vein belongs at the temple — just above and outside
        // the right eye — throbbing in place.
        draw_anger_(it, 256, 60,
                    10 + (int)(std::sin(effect_phase_ * 4.0f * (float)M_PI) * 2.0f));
        break;
      case Effect::Sweat:
        // Anchored next to the face — it has to sit close to read as a
        // sweat drop. Bobs gently with the breath instead of rising.
        draw_sweat_(it, 288, 78 - (int)(3 * breath_),
                    6 - (int)(6 * 0.15f * breath_));
        break;
      case Effect::Chill:
        // Gloom lines hang over the brow (above the right eye), not in
        // the empty corner — that's where the manga mark lives.
        draw_chill_(it, 216, 14);
        break;
      case Effect::Bubbles:
        draw_bubbles_(it);
        break;
      default:
        break;
    }
  }

  // Loop-progress helper for marks with several staggered glyphs (only
  // where plurality IS the motif, like the bubble chain).
  float rise_phase_(int i, int n) {
    float p = effect_phase_ + (float)i / (float)n;
    return p - (int)p;
  }

  // Two eighth notes dancing in place: side-to-side horizontal sway,
  // out of phase with each other — no drifting away from the face.
  void draw_music_(esphome::display::Display &it) {
    const float w = 4.0f * (float)M_PI;  // two sways per 3 s loop
    draw_note_(it, 262 + (int)(std::sin(effect_phase_ * w) * 5.0f), 58);
    draw_note_(it, 288 - (int)(std::sin(effect_phase_ * w) * 5.0f), 78);
  }

  void draw_note_(esphome::display::Display &it, int x, int y) {
    it.filled_circle(x, y, 3, palette_.primary);                  // head
    it.filled_rectangle(x + 2, y - 12, 2, 12, palette_.primary);  // stem
    it.line(x + 3, y - 12, x + 8, y - 8, palette_.primary);       // flag
  }

  // "ZZz" written out horizontally like text — bottoms aligned, growing
  // left to right, the letters appearing one after another over the
  // loop, each with a gentle bob. (Our replacement for the original's
  // sleep bubbles as Sleepy's mark — those remain as Effect::Bubbles.)
  void draw_zzz_(esphome::display::Display &it) {
    static constexpr int kX[3] = {236, 250, 266};
    static constexpr int kS[3] = {8, 11, 14};
    const int visible = 1 + std::min(2, (int)(effect_phase_ * 3.0f));
    for (int i = 0; i < visible; i++) {
      const int bob = (int)(
          std::sin((effect_phase_ + i * 0.25f) * 2.0f * (float)M_PI) * 2.0f);
      draw_z_(it, kX[i], 52 - kS[i] + bob, kS[i]);
    }
  }

  // A 2px-thick "Z" glyph drawn with lines; (x, y) is the top-left, s the size.
  void draw_z_(esphome::display::Display &it, int x, int y, int s) {
    for (int t = 0; t < 2; t++) {
      it.horizontal_line(x, y + t, s, palette_.primary);
      it.horizontal_line(x, y + s - 2 + t, s, palette_.primary);
      it.line(x + s - 1, y + t, x, y + s - 2 + t, palette_.primary);
    }
  }

  void draw_heart_(esphome::display::Display &it, int x, int y, int r) {
    it.filled_circle(x - r / 2, y, r / 2, palette_.primary);
    it.filled_circle(x + r / 2, y, r / 2, palette_.primary);
    const int a = (int)((std::sqrt(2.0f) * r) / 4.0f);
    fill_triangle_(it, x, y, x - r / 2 - a, y + a, x + r / 2 + a, y + a,
                   palette_.primary);
    fill_triangle_(it, x, y + r / 2 + 2 * a, x - r / 2 - a, y + a,
                   x + r / 2 + a, y + a, palette_.primary);
  }

  // (r stays >= 9 so the background carve that splits the cross into
  // four corners remains visible.)
  void draw_anger_(esphome::display::Display &it, int x, int y, int r) {
    it.filled_rectangle(x - r / 3, y - r, (r * 2) / 3, r * 2, palette_.primary);
    it.filled_rectangle(x - r, y - r / 3, r * 2, (r * 2) / 3, palette_.primary);
    it.filled_rectangle(x - r / 3 + 2, y - r, (r * 2) / 3 - 4, r * 2,
                        palette_.background);
    it.filled_rectangle(x - r, y - r / 3 + 2, r * 2, (r * 2) / 3 - 4,
                        palette_.background);
  }

  void draw_sweat_(esphome::display::Display &it, int x, int y, int r) {
    it.filled_circle(x, y, r, palette_.primary);
    const int a = (int)((std::sqrt(3.0f) * r) / 2.0f);
    fill_triangle_(it, x, y - r * 2, x - a, y - r / 2, x + a, y - r / 2,
                   palette_.primary);
  }

  // Gloom/chill: three bars hanging down over the brow, running a
  // staggered wave instead of the original's static lengths.
  void draw_chill_(esphome::display::Display &it, int x, int spacing) {
    for (int i = 0; i < 3; i++) {
      const float ph = (effect_phase_ + i * 0.33f) * 2.0f * (float)M_PI;
      const int h = 18 + (int)(std::sin(ph) * 6.0f);
      it.filled_rectangle(x + i * spacing, 0, 3, h, palette_.primary);
    }
  }

  // A chain of two sleep bubbles floating up with a pronounced
  // horizontal wobble — the sideways dance is the motion, the rise is
  // slow. Replaces the original's static two-ring mark, which read
  // poorly at this resolution.
  void draw_bubbles_(esphome::display::Display &it) {
    for (int i = 0; i < 2; i++) {
      const float p = rise_phase_(i, 2);
      const int r = 2 + (int)(p * 6.0f);
      const int x = 284 + (int)(std::sin(p * 4.0f * (float)M_PI) * 5.0f);
      const int y = 80 - (int)(p * 48.0f);
      it.circle(x, y, r, palette_.primary);
    }
  }

  // xorshift32 — deterministic, no libc rand() on the render path.
  float frand_() {
    rng_ ^= rng_ << 13;
    rng_ ^= rng_ >> 17;
    rng_ ^= rng_ << 5;
    return (float)(rng_ & 0xFFFFFF) / 16777216.0f;
  }

  static esphome::Color unpack_(uint32_t rgb) {
    return esphome::Color((uint8_t)(rgb >> 16), (uint8_t)(rgb >> 8), (uint8_t)rgb);
  }

  enum class IdleState : uint8_t { Awake, Sleeping, Waking };

  Expression   expression_{Expression::Neutral};
  Effect       effect_{Effect::Auto};
  float        effect_phase_{0.0f};
  bool         idle_{false};
  IdleState    idle_state_{IdleState::Awake};
  uint32_t     idle_sleep_after_{300000};    // 5 min
  uint32_t     idle_sleep_duration_{120000}; // 2 min
  uint32_t     last_activity_ms_{0};
  uint32_t     idle_until_ms_{0};
  uint32_t     mood_until_ms_{0};
  float        breath_phase_{0.0f};
  uint32_t     last_ms_{0};
  bool         eye_open_{true};
  float        blink_{1.0f};
  float        breath_{0.0f};
  bool         speaking_{false};
  bool         listening_{false};
  float        speak_{0.0f};
  float        speak_target_{0.0f};
  float        gaze_h_{0.0f};
  float        gaze_v_{0.0f};
  uint32_t     next_blink_ms_{0};
  uint32_t     next_saccade_ms_{0};
  uint32_t     next_syllable_ms_{0};
  uint32_t     rng_{0x6C078965};
  ColorPalette palette_{};
  FaceGeometry geom_{};
};

}  // namespace stackchan_avatar
