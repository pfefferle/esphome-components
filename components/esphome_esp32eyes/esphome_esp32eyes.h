// esphome_esp32eyes.h
//
// ESPHome adapter for esp32-eyes
// (https://github.com/playfultechnology/esp32-eyes, the other sources in
// this directory, included unchanged).
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
#include <cstdlib>
#include <new>
#include "Face.h"

namespace esp32eyes {

// esp32-eyes on an ESPHome display. The upstream Face needs the screen
// size when it is created, so it is created on the first frame:
//
//   static esp32eyes::Eyes eyes;
//   if (eyes.begin(it, 48)) {        // true once, on the first frame
//     eyes.face().RandomBehavior = true;
//   }
//   eyes.draw(it);
class Eyes {
 public:
  // Creates the face on the first call and returns true then, so one-time
  // settings can follow. eye_size is the eye width/height in pixels.
  bool begin(esphome::display::Display &it, uint16_t eye_size) {
    u8g2.set_display(&it);
    if (face_ != nullptr) return false;
    // Upstream leaves several fields uninitialized (EyeConfig is a plain
    // struct, the transition target is only set by the first expression).
    // With garbage in them the corner radii get huge and drawing a frame
    // takes long enough to trip the watchdog. Start from zeroed memory.
    void *mem = calloc(1, sizeof(Face));
    if (mem == nullptr) return false;
    face_ = new (mem) Face(it.get_width(), it.get_height(), eye_size);
    // As the upstream example sketch does right after creating the face
    face_->Expression.GoTo_Normal();
    return true;
  }

  // The face exists from the first frame on; check before using face()
  // outside the display lambda (buttons, selects).
  bool ready() const { return face_ != nullptr; }
  Face &face() { return *face_; }

  void set_colors(esphome::Color background, esphome::Color main) {
    u8g2.set_colors(background, main);
  }
  // Eye size: 1.0 is upstream's 40x40px eyes, scaled around the screen center.
  void set_scale(float scale) { u8g2.set_scale(scale); }

  // Once per display update: runs behavior, look and blink, then draws.
  void draw(esphome::display::Display &it) {
    u8g2.set_display(&it);
    if (face_ != nullptr) face_->Update();
  }

 protected:
  Face *face_{nullptr};
};

}  // namespace esp32eyes
