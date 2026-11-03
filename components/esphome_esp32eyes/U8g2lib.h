// U8g2lib.h
//
// Stand-in for the u8g2 display class esp32-eyes draws with, so its
// sources build unchanged. Draws on an ESPHome display::Display instead.
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public
// License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "esphome/core/color.h"
#include "esphome/components/display/display.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

#define U8G2_R0 0
#define U8X8_PIN_NONE 255

// esp32-eyes defines one global `u8g2` of this type (Face.cpp) and only
// uses the calls below. Color 0 is the background, 1 the eyes.
//
// set_scale() scales everything around the screen center. esp32-eyes
// draws its eyes at a fixed 40x40px, so this is how their size changes.
class U8G2_SSD1306_128X64_NONAME_F_HW_I2C {
 public:
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C(int rotation = U8G2_R0, int reset = U8X8_PIN_NONE,
                                      int clock = U8X8_PIN_NONE, int data = U8X8_PIN_NONE) {}

  // --- ESPHome side
  void set_display(esphome::display::Display *it) { it_ = it; }
  void set_colors(esphome::Color background, esphome::Color main) {
    background_ = background;
    main_ = main;
  }
  void set_scale(float scale) { scale_ = scale > 0.0f ? scale : 1.0f; }

  // --- the u8g2 calls esp32-eyes makes
  void setI2CAddress(uint8_t) {}
  bool begin() { return true; }
  void clearBuffer() {
    if (it_ != nullptr) it_->fill(background_);
  }
  void sendBuffer() {}  // ESPHome sends the buffer itself after the lambda
  void setDrawColor(uint8_t color) { color_ = color; }
  void drawHLine(int x, int y, int w) { drawBox(x, y, w, 1); }
  void drawBox(int x, int y, int w, int h) {
    if (it_ == nullptr || w <= 0 || h <= 0) return;
    const int x0 = tx_(x), y0 = ty_(y);
    const int sw = std::max(1, tx_(x + w) - x0), sh = std::max(1, ty_(y + h) - y0);
    if (sh == 1) {
      it_->horizontal_line(x0, y0, sw, current_());
    } else {
      it_->filled_rectangle(x0, y0, sw, sh, current_());
    }
  }
  void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2) {
    if (it_ != nullptr)
      it_->filled_triangle(tx_(x0), ty_(y0), tx_(x1), ty_(y1), tx_(x2), ty_(y2), current_());
  }

 protected:
  esphome::Color current_() const { return color_ ? main_ : background_; }
  // Scale around the screen center; identity at scale 1.
  int tx_(int x) const {
    const float c = it_->get_width() / 2.0f;
    return (int) std::lround(c + (x - c) * scale_);
  }
  int ty_(int y) const {
    const float c = it_->get_height() / 2.0f;
    return (int) std::lround(c + (y - c) * scale_);
  }

  esphome::display::Display *it_{nullptr};
  esphome::Color background_{0, 0, 0};
  esphome::Color main_{255, 255, 255};
  uint8_t color_{1};
  float scale_{1.0f};
};
