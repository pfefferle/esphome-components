# esphome_avatar

A header-only Stack-chan-style avatar face for ESPHome. Renders eyes
and a wide Stack-chan mouth on any `display::Display`-compatible panel.
Expressions are carved into the eye shape (no eyebrows): triangle lids
for angry/sad, a happy crescent, a droopy half-disc for sleepy.
Blink timing, saccade gaze jumps, breath, and geometry are ported 1:1
from the original.

Ported from [stack-chan/m5stack-avatar][upstream] and retargeted at
ESPHome's display API. Real lip sync is intentionally omitted —
`set_speaking(true)` drives random syllable-like mouth levels instead,
standing in for the original's audio-level-driven lipSync task. On top
of the port there are animated [effect marks](#effects) (music notes,
"ZZz", heart, …), a `set_listening(true)` state (wide attentive eyes,
mouth firmly closed), and an [idle mode](#idle-mode) that keeps the
face alive on its own.

[upstream]: https://github.com/stack-chan/m5stack-avatar

## Quick start

1. Copy the starter config [`examples/avatar.yaml`](../../examples/avatar.yaml)
   (M5Stack Core, ESP32 + ILI9341 320×240) into your ESPHome config
   directory. It exposes HA selects for expression/effect, switches for
   idle/speaking/listening, and an optional media_player binding. The
   avatar is pulled from GitHub automatically — no need to download
   `esphome_avatar.h` yourself.
2. Create `secrets.yaml` next to it:

   ```yaml
   wifi_ssid: "your-ssid"
   wifi_password: "your-password"
   api_encryption_key: "<32-byte base64, run `esphome config avatar.yaml` to scaffold>"
   ota_password: "your-ota-password"
   ```

3. Flash:

   ```bash
   esphome run avatar.yaml
   ```

## Use in an existing config

```yaml
packages:
  avatar:
    url: https://github.com/pfefferle/esphome-components
    files: [package.yaml]
    ref: main
```

or wire the external component up directly:

```yaml
external_components:
  - source: github://pfefferle/esphome-components@main
    components: [esphome_avatar]

esphome_avatar:
```

Either way, `stackchan_avatar::Avatar` is available in `display:`
lambdas afterwards. (`esphome: includes:` can't ship C++ from a remote
package, which is why the header travels as an external component.)

## Other boards

The avatar code is panel-agnostic. To target a different M5Stack /
display, swap the `esp32:` board, the `spi:` pin block, and the
`display:` platform in `examples/avatar.yaml`. Geometry defaults assume a
320×240 panel — for smaller displays, supply a scaled `FaceGeometry`.

## API

```cpp
namespace stackchan_avatar {

enum class Expression : uint8_t {
  Neutral, Happy, Sad, Angry, Sleepy, Doubt
};

enum class Effect : uint8_t {
  Auto,     // follow the expression (original behavior), default
  None,     // hide the mark
  Music,    // a little melody of eighth notes, written out note by note
  Zzz,      // "ZZz" written out above the head, letter by letter
  Heart, Anger, Sweat, Chill, Bubbles   // original Effect.h marks
};

class Avatar {
  // State
  void set_expression(Expression e);
  void set_speaking(bool s);   // animates the mouth (random syllables)
  void set_listening(bool l);  // attentive eyes, mouth stays closed
  void set_effect(Effect e);   // top-right mark, Auto = follow expression

  // Idle mode: self-running mood wander + doze/wake cycle
  void set_idle(bool i);
  void set_idle_sleep_after(uint32_t ms);     // default 5 min, 0 = never
  void set_idle_sleep_duration(uint32_t ms);  // default 2 min

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

## Idle mode

`set_idle(true)` turns the avatar into a self-running little show —
nice to leave on a desk. Neither the original lib nor the stack-chan
firmware has this built in (the lib's expression changes are app-driven;
the firmware's face demo cycles emotions on a plain timer):

- **Mood wander** — always starts neutral, then picks a calm
  expression every 15–45 s, weighted towards neutral (50 %),
  happy (30 %), doubt (20 %).
- **Doze off** — after 5 min without speaking/listening it falls
  asleep: Sleepy face, floating "ZZz", breathing slows to a deep
  ~6.6 s cycle, gaze settles low, saccades stop.
- **Wake up** — after a 2 min nap (or immediately when it has to speak
  or listen) it wakes with a bright-eyed happy stretch, then returns
  to the mood wander.

While idle mode is on it owns the expression — `set_expression()` is
overridden each frame. Speaking and listening still work normally and
count as activity. Timings are configurable via
`set_idle_sleep_after()` / `set_idle_sleep_duration()`.

## Effects

A small mark drawn near the top-right of the face, ported from the
original `Effect.h` and extended. With `Effect::Auto` (the default) it
follows the expression, exactly like the original:

| Expression | Auto mark |
|------------|-----------|
| Happy      | floating heart |
| Angry      | throbbing anger vein at the temple |
| Sad        | gloom lines over the brow |
| Doubt      | sweat drop |
| Sleepy     | "ZZz" (original's bubbles: `Effect::Bubbles`) |

`Effect::Music` shows a little melody of eighth notes — the starter
config switches to it automatically while the bound media_player is
playing, and back to `auto` when it stops. `Effect::None` hides the
mark.

All marks are animated, with mostly horizontal motion: notes and
"ZZz" write themselves out glyph by glyph on a 3 s loop — the three
notes appear one after another, each at its own pitch height — while
heart and bubbles float up with a pronounced sideways wobble. Three
marks are anchored on the face, where they need to sit to read right:
the sweat drop bobs next to the temple, the anger vein throbs above
the right eye, and the gloom lines hang over the brow, running a
staggered wave.

## Driving from Home Assistant

The starter `examples/avatar.yaml` exposes:

- `select.avatar_expression` — `neutral | happy | sad | angry | sleepy | doubt`
- `select.avatar_effect` — `auto | none | music | zzz | heart | anger | sweat | chill | bubbles`
- `switch.avatar_idle` — self-running mood wander + doze/wake (on by
  default; overrides the expression select while on)
- `switch.avatar_speaking` — toggles the speech (mouth) animation
- `switch.avatar_listening` — attentive face, mouth stays closed

The starter's `display:` lambda already wires all of these in. The
media_player binding at the bottom of `avatar.yaml` additionally
flips speaking + the music effect automatically while a bound HA
media_player is playing — set its `entity_id`, or remove the block.

## Credits

- Original avatar geometry, expression presets, and design vocabulary:
  [stack-chan/m5stack-avatar][upstream] (MIT).
- ESPHome retarget and animation glue: this repo.
