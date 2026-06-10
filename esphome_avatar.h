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
//
// Deviations:
//   - lip sync: the original samples real TTS audio levels; here
//     `set_speaking()` drives random syllable-like mouth levels instead
//   - `set_listening()`: attentive state the original doesn't have —
//     wide steady eyes, raised gaze, mouth stays closed
//   - Doubt: the original expressed doubt via eyebrows; here it gets
//     half-lidded flat-top eyes instead
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
    const float t = now_ms / 1000.0f;

    // --- breath: ~3.3 s sine (original: 100 ticks at 33 ms)
    breath_ = std::sin(t * 2.0f * (float)M_PI / 3.3f);

    // --- saccades: gaze jumps to a random direction, then holds.
    // Original: interval 500 + 100*random(20) ms, gaze in [-1, 1].
    // While listening, the gaze locks on slightly raised — focused on
    // the person talking instead of wandering.
    if (listening_) {
      gaze_h_ = 0.0f;
      gaze_v_ = -0.4f;
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
        carve_lid_(it, cx - r, cx + r, cy - r, r,
                   /*apex_right=*/is_left == sad);
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

  // Background triangle (x0,y0)-(x1,y0)-(apex,y0+r) via scanlines —
  // ESPHome's filled_triangle is recent; this keeps the minimum version low.
  void carve_lid_(esphome::display::Display &it, int x0, int x1, int y0,
                  int r, bool apex_right) {
    for (int i = 0; i <= r; i++) {
      const float f = (float)i / (float)r;
      const int shrink = (int)((x1 - x0) * f);
      const int xl = apex_right ? x0 + shrink : x0;
      const int xr = apex_right ? x1 : x1 - shrink;
      it.horizontal_line(xl, y0 + i, xr - xl + 1, palette_.background);
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

  Expression   expression_{Expression::Neutral};
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
