# esphome-avatar

A header-only Stack-chan-style avatar face for ESPHome. Renders eyes
and a wide Stack-chan mouth on any `display::Display`-compatible panel.
Expressions are carved into the eye shape (no eyebrows): triangle lids
for angry/sad, a happy crescent, a droopy half-disc for sleepy.
Blink timing, saccade gaze jumps, breath, and geometry are ported 1:1
from the original.

Ported from [stack-chan/m5stack-avatar][upstream] and retargeted at
ESPHome's display API. Real lip sync is intentionally omitted —
`set_speaking(true)` drives random syllable-like mouth levels instead,
standing in for the original's audio-level-driven lipSync task. There
is also a `set_listening(true)` state the original doesn't have: wide
attentive eyes with a steady, slightly raised gaze — the mouth stays
firmly closed while listening.

[upstream]: https://github.com/stack-chan/m5stack-avatar

## Files

- `esphome_avatar.h` — the avatar library (header-only).
- `package.yaml` — ESPHome package partial that ships the header.
  Consumers reference this from GitHub; no local copy needed.
- `esphome.yaml` — starter config for an M5Stack Core (ESP32 + ILI9341
  320×240). Pulls the avatar from GitHub via `packages:`. Includes
  optional HA `select` for expression and `switch` for speaking state.

## Quick start

1. Copy `esphome.yaml` into your ESPHome config directory. The avatar
   header is pulled from GitHub automatically — no need to download
   `esphome_avatar.h` yourself.
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
  void set_speaking(bool s);   // animates the mouth (random syllables)
  void set_listening(bool l);  // attentive eyes, mouth stays closed

  // Per-frame: drive blink/saccades/breath/speech from millis()
  void update_animations(uint32_t now_ms);

  // Manual override of any animation axis (skip update_animations)
  void set_blink_phase(float p);        // 0 closed .. 1 open
  void set_breath_phase(float p);       // -1..1 sine
  void set_speak_phase(float p);        // 0..1 mouth-open
  void set_gaze(float v, float h);      // -1..1 each, eye offset

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

Expressions are eye shapes, carved out of the filled eye circle with
background-colored shapes in `Avatar::draw_eye_()` — a direct port of
the original `Eye::draw()`:

| Expression | Eye shape |
|------------|-----------|
| Neutral    | full circle |
| Happy      | upper crescent (`^_^`) |
| Sad        | slanted lid, low corner outward |
| Angry      | slanted lid, low corner inward |
| Sleepy     | droopy lower half-disc |
| Doubt      | half-lidded flat top |

The mouth is the original Stack-chan rectangle: a wide thin line
(90×4 px) when closed that narrows and opens vertically while speaking
(up to 50×60 px). Sizes and positions live in `FaceGeometry`; the eye
carving logic is the place to tweak or add expressions.

## Driving from Home Assistant

The starter `esphome.yaml` exposes:

- `select.avatar_expression` — `neutral | happy | sad | angry | sleepy | doubt`
- `switch.avatar_speaking` — toggles the speech (mouth) animation
- `switch.avatar_listening` — attentive face, mouth stays closed

Replace the basic `display:` lambda with the richer one shown in
comments at the bottom of `esphome.yaml` to wire the HA controls into
the avatar.

## Credits

- Original avatar geometry, expression presets, and design vocabulary:
  [stack-chan/m5stack-avatar][upstream] (MIT).
- ESPHome retarget and animation glue: this repo.
