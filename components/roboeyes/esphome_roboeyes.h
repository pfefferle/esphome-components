// esphome_roboeyes.h
//
// ESPHome adapter for FluxGarage RoboEyes
// (https://github.com/FluxGarage/RoboEyes, FluxGarage_RoboEyes.h in this
// directory, included unchanged).
//
// Copyright (C) 2026 Matthias Pfefferle
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//
// RoboEyes draws through an Adafruit GFX style display object and uses a
// few Arduino helpers. This file provides both on top of ESPHome's
// display::Display, so the upstream header can stay as it is.

#pragma once

#include "esphome/core/color.h"
#include "esphome/components/display/display.h"
#include <algorithm>
#include <cstdint>

#include "Arduino_compat.h"
#include "FluxGarage_RoboEyes.h"

// RoboEyes defines short global macros (N, E, S, W, ON, OFF, DEFAULT, ...).
// They are only needed while its header is parsed; drop them so they
// cannot clash with ESPHome's generated code, and offer them as constants.
#undef DEFAULT
#undef TIRED
#undef ANGRY
#undef HAPPY
#undef ON
#undef OFF
#undef N
#undef NE
#undef E
#undef SE
#undef S
#undef SW
#undef W
#undef NW

namespace roboeyes {

// Moods (setMood)
static constexpr unsigned char DEFAULT = 0;
static constexpr unsigned char TIRED = 1;
static constexpr unsigned char ANGRY = 2;
static constexpr unsigned char HAPPY = 3;
// Switches
static constexpr bool ON = true;
static constexpr bool OFF = false;
// Positions (setPosition); DEFAULT = middle center
static constexpr unsigned char N = 1;
static constexpr unsigned char NE = 2;
static constexpr unsigned char E = 3;
static constexpr unsigned char SE = 4;
static constexpr unsigned char S = 5;
static constexpr unsigned char SW = 6;
static constexpr unsigned char W = 7;
static constexpr unsigned char NW = 8;

// The Adafruit GFX calls RoboEyes uses, drawn on an ESPHome display.
// RoboEyes paints with color 0 (background) and 1 (main).
class Gfx {
 public:
  void set_display(esphome::display::Display *it) { it_ = it; }
  void set_colors(esphome::Color background, esphome::Color main) {
    background_ = background;
    main_ = main;
  }

  void clearDisplay() { it_->fill(background_); }
  void display() {}  // ESPHome sends the buffer itself after the lambda

  void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                     uint16_t color) {
    if (w <= 0 || h <= 0) return;
    const esphome::Color c = color_(color);
    r = std::max<int16_t>(0, std::min<int16_t>(r, std::min(w, h) / 2));
    if (r == 0) {
      it_->filled_rectangle(x, y, w, h, c);
      return;
    }
    it_->filled_rectangle(x + r, y, w - 2 * r, h, c);
    it_->filled_rectangle(x, y + r, w, h - 2 * r, c);
    it_->filled_circle(x + r, y + r, r, c);
    it_->filled_circle(x + w - 1 - r, y + r, r, c);
    it_->filled_circle(x + r, y + h - 1 - r, r, c);
    it_->filled_circle(x + w - 1 - r, y + h - 1 - r, r, c);
  }

  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2,
                    int16_t y2, uint16_t color) {
    it_->filled_triangle(x0, y0, x1, y1, x2, y2, color_(color));
  }

 protected:
  esphome::Color color_(uint16_t c) const { return c ? main_ : background_; }

  esphome::display::Display *it_{nullptr};
  esphome::Color background_{0, 0, 0};
  esphome::Color main_{255, 255, 255};
};

// RoboEyes on an ESPHome display. Call the RoboEyes setters through
// eyes(), then draw(it) once per display update. Set the sizes (setWidth,
// setHeight, setBorderradius, setSpacebetween) only once: they also reset
// the animation targets, so calling them every frame cancels every blink.
class Face {
 public:
  Face() : eyes_(gfx_) {}

  RoboEyes<Gfx> &eyes() { return eyes_; }
  void set_colors(esphome::Color background, esphome::Color main) {
    gfx_.set_colors(background, main);
  }

  void draw(esphome::display::Display &it) {
    gfx_.set_display(&it);
    if (!started_) {
      start_(it);
      started_ = true;
    }
    // Every frame: ESPHome clears the buffer before the lambda, so the
    // frame-rate limit in RoboEyes::update() would leave empty frames.
    // The display's update_interval sets the frame rate instead.
    eyes_.drawEyes();
  }

 protected:
  // RoboEyes computes its start position and eyelid sizes once, for a
  // 128x64 screen with 36px eyes. Redo that for this screen and the
  // sizes set before the first frame, then start centered.
  void start_(esphome::display::Display &it) {
    auto &e = eyes_;
    e.begin(it.get_width(), it.get_height(), 100);
    e.eyeLwidthCurrent = e.eyeLwidthDefault;
    e.eyeRwidthCurrent = e.eyeRwidthDefault;
    e.spaceBetweenCurrent = e.spaceBetweenDefault;
    e.eyelidsHeightMax = e.eyeLheightDefault / 2;
    e.eyelidsHappyBottomOffsetMax = (e.eyeLheightDefault / 2) + 3;
    e.setPosition(DEFAULT);
    e.eyeLx = e.eyeLxNext;
    e.eyeLy = e.eyeLyNext;
    e.eyeRx = e.eyeLx + e.eyeLwidthCurrent + e.spaceBetweenCurrent;
    e.eyeRy = e.eyeLy;
  }

  Gfx gfx_;
  RoboEyes<Gfx> eyes_;
  bool started_{false};
};

}  // namespace roboeyes
