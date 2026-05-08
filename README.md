# esphome-avatar

A header-only Stack-chan-style avatar face for ESPHome. Renders eyes,
eyebrows, and a mouth on any `display::Display`-compatible panel, with
six expression presets, automatic blink/breath, and a generic speech
open/close loop.

Ported from [stack-chan/m5stack-avatar][upstream] (the geometry and
expression presets) and retargeted at ESPHome's display API. Lip sync
is intentionally omitted — `set_speaking(true)` drives a layered-sine
jabber animation instead.

[upstream]: https://github.com/stack-chan/m5stack-avatar

## Files

- `esphome_avatar.h` — the avatar library (header-only).
- `esphome.yaml` — starter config for an M5Stack Core (ESP32 + ILI9341
  320×240). Includes optional HA `select` for expression and `switch`
  for speaking state.

## Quick start

1. Drop `esphome_avatar.h` and `esphome.yaml` into a fresh ESPHome
   config directory (or use this directory directly).
2. Create `secrets.yaml` next to `esphome.yaml`:

   ```yaml
   wifi_ssid: "your-ssid"
   wifi_password: "your-password"
   api_encryption_key: "<32-byte base64, run `esphome config esphome.yaml` to scaffold>"
   ota_password: "your-ota-password"
   ```

3. Flash:

   ```bash
   esphome run esphome.yaml
   ```

## Other boards

The avatar code is panel-agnostic. To target a different M5Stack /
display, swap the `esp32:` board, the `spi:` pin block, and the
`display:` platform in `esphome.yaml`. Geometry defaults assume a
320×240 panel — for smaller displays, supply a scaled `FaceGeometry`.

## API

```cpp
namespace stackchan_avatar {

enum class Expression : uint8_t {
  Neutral, Happy, Sad, Angry, Sleepy, Doubt
};

class Avatar {
  // State
  void set_expression(Expression e);
  void set_speaking(bool s);

  // Per-frame: drive blink/breath/speech from millis()
  void update_animations(uint32_t now_ms);

  // Manual override of any animation axis (skip update_animations)
  void set_blink_phase(float p);   // 0 closed .. 1 open
  void set_breath_phase(float p);  // -1..1 sine
  void set_speak_phase(float p);   // 0..1 mouth-open

  // Palette (background, primary, secondary)
  void set_palette(const ColorPalette &p);
  void set_background(esphome::Color c);
  void set_primary(esphome::Color c);
  void set_secondary(esphome::Color c);
  void set_background_rgb(uint32_t rgb);  // 0xRRGGBB
  void set_primary_rgb(uint32_t rgb);
  void set_secondary_rgb(uint32_t rgb);

  // Geometry (positions, sizes — see FaceGeometry struct)
  void set_geometry(const FaceGeometry &g);

  // Render — call once per display update
  void draw(esphome::display::Display &it);
};

}
```

Minimal `display:` lambda:

```yaml
display:
  - platform: ili9xxx
    # ...
    lambda: |-
      static stackchan_avatar::Avatar avatar;
      avatar.update_animations(millis());
      avatar.draw(it);
```

## Customizing expressions

Each preset is a row in `kExpressions[]` inside `esphome_avatar.h`:

```cpp
struct ExpressionParams {
  float    eye_open_ratio;    // 0..1, multiplied with blink
  int8_t   brow_left_angle;   // degrees, +ve = inner-up
  int8_t   brow_right_angle;
  int8_t   brow_offset_y;
  float    mouth_open_ratio;  // baseline open amount
  uint16_t mouth_width;
  bool     mouth_curve_down;  // sad/angry curve when closed
};
```

Tweak the values in place — they're the knob you'll want to play with
most. Add new entries by extending the enum and the array together.

## Driving from Home Assistant

The starter `esphome.yaml` exposes:

- `select.avatar_expression` — `neutral | happy | sad | angry | sleepy | doubt`
- `switch.avatar_speaking` — toggles the speech animation

Replace the basic `display:` lambda with the richer one shown in
comments at the bottom of `esphome.yaml` to wire the HA controls into
the avatar.

## Credits

- Original avatar geometry, expression presets, and design vocabulary:
  [stack-chan/m5stack-avatar][upstream] (MIT).
- ESPHome retarget and animation glue: this repo.
