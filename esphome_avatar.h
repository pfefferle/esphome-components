// esphome_avatar.h
//
// Copyright (c) 2018 Shinya Ishikawa
// Copyright (c) 2026 Matthias Pfefferle
// SPDX-License-Identifier: MIT
//
// Header-only port of the geometry + expression presets from
// stack-chan/m5stack-avatar (https://github.com/stack-chan/m5stack-avatar),
// retargeted at ESPHome's display::Display API.
// Lip sync intentionally omitted — `set_speaking()` drives a generic
// open/close loop instead.
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

// Per-expression deltas applied on top of FaceGeometry.
// Ported semantically from m5stack-avatar Face::draw() and the Eye/Mouth/
// Eyeblow update logic — these are the parameter sets the original picks
// from when setExpression() is called.
struct ExpressionParams {
  float   eye_open_ratio;     // 0..1, multiplied with blink phase
  int8_t  brow_left_angle;    // degrees, +ve = inner-up (happy/surprised)
  int8_t  brow_right_angle;   // mirrored for the other brow
  int8_t  brow_offset_y;      // shift brows up (-) / down (+)
  float   mouth_open_ratio;   // 0..1, baseline open amount
  uint16_t mouth_width;       // px
  bool    mouth_curve_down;   // sad/angry: invert mouth curve when closed
};

// The six default presets. Tweak freely — these are the knob you'll
// want to play with most.
static constexpr ExpressionParams kExpressions[6] = {
  /* Neutral */ { 1.00f,   0,   0,  0, 0.00f, 60, false },
  /* Happy   */ { 0.50f,  10, -10, -3, 0.30f, 80, false },
  /* Sad     */ { 1.00f, -15,  15,  0, 0.00f, 50, true  },
  /* Angry   */ { 1.00f, -20,  20, -5, 0.00f, 40, true  },
  /* Sleepy  */ { 0.10f,   0,   0,  3, 0.00f, 50, false },
  /* Doubt   */ { 0.90f,  15,  -5,  0, 0.10f, 50, false },
};

// Three-slot palette mirroring the original m5stack-avatar ColorPalette:
//   background — fills the screen each frame
//   primary    — eyes, eyebrows, mouth fill
//   secondary  — reserved for accents (iris, mouth highlight) — used by
//                draw_mouth_ when the mouth is open as a subtle inner color
struct ColorPalette {
  esphome::Color background{0,   0,   0};
  esphome::Color primary   {255, 255, 255};
  esphome::Color secondary {255, 255, 255};
};
// 320x240 face layout, ported from the Face() default constructor.
// Coords are (x, y) in screen space; original library uses (top, left)
// BoundingRects — these are the same values transposed.
struct FaceGeometry {
  // Eyes (viewer's left = smaller x)
  uint16_t left_eye_x   = 90;
  uint16_t left_eye_y   = 93;
  uint16_t right_eye_x  = 230;
  uint16_t right_eye_y  = 95;
  uint16_t eye_radius   = 8;

  // Eyebrows
  uint16_t left_brow_x  = 95;
  uint16_t left_brow_y  = 68;
  uint16_t right_brow_x = 230;
  uint16_t right_brow_y = 73;
  uint16_t brow_width   = 32;
  uint16_t brow_height  = 2;

  // Mouth
  uint16_t mouth_x         = 163;
  uint16_t mouth_y         = 148;
  uint16_t mouth_min_h     = 5;
  uint16_t mouth_max_h     = 60;
};

class Avatar {
 public:
  // --- state setters, all designed to be called from interval/lambda ---
  void set_expression(Expression e) { expression_ = e; }
  void set_blink_phase(float p)     { blink_ = std::clamp(p, 0.0f, 1.0f); }
  void set_breath_phase(float p)    { breath_ = std::clamp(p, -1.0f, 1.0f); }
  void set_speaking(bool s)         { speaking_ = s; }
  void set_speak_phase(float p)     { speak_ = std::clamp(p, 0.0f, 1.0f); }

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

  // Convenience: drive blink, breath, and speech animation from a single
  // millis() value. Call once per frame before draw(). If you want manual
  // control over any axis, call set_blink_phase/set_breath_phase/
  // set_speak_phase yourself instead.
  void update_animations(uint32_t now_ms) {
    // --- breath: 3s sine, identical to the original m5stack-avatar task
    const float t = now_ms / 1000.0f;
    breath_ = std::sin(t * 2.0f * (float)M_PI / 3.0f);

    // --- blink: every ~4s, 120ms close-open, snappy
    const uint32_t cycle = 4000;
    const uint32_t blink_dur = 240;  // ms total (down + up)
    uint32_t pos = now_ms % cycle;
    if (pos < blink_dur) {
      // triangle wave 1 -> 0 -> 1
      float x = (float)pos / (float)blink_dur;     // 0..1
      blink_ = (x < 0.5f) ? (1.0f - x * 2.0f) : (x * 2.0f - 1.0f);
    } else {
      blink_ = 1.0f;
    }

    // --- speech: layered sines for natural-looking jabber
    if (speaking_) {
      // Fast carrier (~5 Hz) modulated by a slower envelope (~1.7 Hz)
      // gives short syllable-like bursts with quiet gaps between.
      float carrier  = 0.5f + 0.5f * std::sin(t * 2.0f * (float)M_PI * 5.0f);
      float envelope = 0.5f + 0.5f * std::sin(t * 2.0f * (float)M_PI * 1.7f);
      // Floor it so envelope drops fully closed during gaps.
      envelope = std::max(0.0f, envelope - 0.25f) * (1.0f / 0.75f);
      speak_ = carrier * envelope;
    } else {
      // Decay quickly when speech ends so mouth closes smoothly.
      speak_ *= 0.6f;
      if (speak_ < 0.02f) speak_ = 0.0f;
    }
  }

  // --- the only call you need from your display lambda ---
  void draw(esphome::display::Display &it) {
    it.fill(palette_.background);
    const auto &p = kExpressions[(uint8_t)expression_];
    const float eye_open = p.eye_open_ratio * blink_;
    const int by = (int)(breath_ * 3.0f);  // subtle vertical breathing

    draw_eye_(it, geom_.left_eye_x,  geom_.left_eye_y  + by, eye_open);
    draw_eye_(it, geom_.right_eye_x, geom_.right_eye_y + by, eye_open);

    draw_brow_(it, geom_.left_brow_x,
               geom_.left_brow_y + by + p.brow_offset_y,
               geom_.brow_width, geom_.brow_height,
               p.brow_left_angle);
    draw_brow_(it, geom_.right_brow_x,
               geom_.right_brow_y + by + p.brow_offset_y,
               geom_.brow_width, geom_.brow_height,
               p.brow_right_angle);

    draw_mouth_(it, p, by);
  }

 protected:
  // Filled ellipse via row scanlines — ESPHome has no native ellipse prim.
  // Cheap enough at avatar scale (eye rx ~8, mouth rx ~40).
  void draw_filled_ellipse_(esphome::display::Display &it,
                            int cx, int cy, int rx, int ry,
                            esphome::Color c) {
    if (rx < 1) rx = 1;
    if (ry < 1) ry = 1;
    const float ry2 = (float)(ry * ry);
    for (int y = -ry; y <= ry; y++) {
      int dx = (int)(rx * std::sqrt(1.0f - (float)(y * y) / ry2));
      it.horizontal_line(cx - dx, cy + y, dx * 2 + 1, c);
    }
  }

  void draw_eye_(esphome::display::Display &it, int cx, int cy, float open) {
    int ry = std::max(1, (int)(geom_.eye_radius * open));
    draw_filled_ellipse_(it, cx, cy, geom_.eye_radius, ry, palette_.primary);
  }

  // Approximate angled brow with a stack of lines (cheap "thick line").
  void draw_brow_(esphome::display::Display &it,
                  int cx, int cy, int w, int h, int angle_deg) {
    const float a = angle_deg * (float)M_PI / 180.0f;
    const int dx = (int)(std::cos(a) * w / 2);
    const int dy = (int)(std::sin(a) * w / 2);
    const int half_h = std::max(1, h / 2);
    for (int t = -half_h; t <= half_h; t++) {
      it.line(cx - dx, cy - dy + t, cx + dx, cy + dy + t, palette_.primary);
    }
  }

  void draw_mouth_(esphome::display::Display &it,
                   const ExpressionParams &p, int by) {
    // Combine baseline open-ratio with active speaking animation.
    float open = p.mouth_open_ratio;
    if (speaking_) open = std::max(open, 0.4f * speak_);

    const int w = (int)p.mouth_width;
    const int h = (int)(geom_.mouth_min_h +
                        (geom_.mouth_max_h - geom_.mouth_min_h) * open);
    const int mx = geom_.mouth_x;
    const int my = geom_.mouth_y + by;

    if (open < 0.05f) {
      // Closed: line, optionally curved down for sad/angry
      if (p.mouth_curve_down) {
        it.line(mx - w/2, my + 2, mx,       my - 2, palette_.primary);
        it.line(mx,       my - 2, mx + w/2, my + 2, palette_.primary);
      } else {
        it.horizontal_line(mx - w/2, my, w, palette_.primary);
      }
    } else {
      // Open: filled primary ellipse with smaller secondary inner ellipse
      // (subtle inner-mouth color when secondary differs from primary)
      draw_filled_ellipse_(it, mx, my + h/4, w/2,        h/2,        palette_.primary);
      if (h > 6 && w > 6) {
        draw_filled_ellipse_(it, mx, my + h/4, w/2 - 3,  h/2 - 2,    palette_.secondary);
      }
    }
  }

  static esphome::Color unpack_(uint32_t rgb) {
    return esphome::Color((uint8_t)(rgb >> 16), (uint8_t)(rgb >> 8), (uint8_t)rgb);
  }

  Expression       expression_{Expression::Neutral};
  float            blink_{1.0f};
  float            breath_{0.0f};
  bool             speaking_{false};
  float            speak_{0.0f};
  ColorPalette     palette_{};
  FaceGeometry     geom_{};
};

}  // namespace stackchan_avatar
