// Arduino_compat.h
//
// Arduino helpers FluxGarage RoboEyes expects, for ESP-IDF builds.
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
// ESPHome's generated esphome.h includes every header of a component in
// name order. This file has to come before FluxGarage_RoboEyes.h, which
// is why its name starts with an uppercase "A".

#pragma once

#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include <cstdint>
#include <cstdlib>

#ifndef USE_ARDUINO
typedef uint8_t byte;
using esphome::millis;
inline long random(long max) {
  return max > 0 ? (long)(esphome::random_uint32() % (uint32_t)max) : 0;
}
inline long random(long min, long max) {
  return max > min ? min + random(max - min) : min;
}
#endif
