// Arduino.h
//
// Stand-in for the Arduino core header, so the esp32-eyes sources (which
// `#include <Arduino.h>`) build unchanged on ESP-IDF. On Arduino builds it
// hands over to the real header.
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

#include "esphome/core/defines.h"

#ifdef USE_ARDUINO
#include_next <Arduino.h>
#else

#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>

using esphome::millis;
using std::max;
using std::min;

inline long random(long max) {
  return max > 0 ? (long)(esphome::random_uint32() % (uint32_t)max) : 0;
}
inline long random(long min, long max) {
  return max > min ? min + random(max - min) : min;
}

#endif
