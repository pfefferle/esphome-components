# PwnStack

A drop-in [ESPHome](https://esphome.io/) package that randomly displays a
[Palnagotchi](https://github.com/viniciusbo/m5-palnagotchi) mood face on
your device's LCD, then hands the screen back to whatever was drawing
before.

Built originally as a fun overlay for the M5Stack
[StackChan](https://github.com/m5stack/esphome-yaml) voice assistant, but it
works on any ESPHome `display:` you can address by `id`.

## What it does

- While your device is idle, every 30s–5min (configurable) a random face
  pops up for a few seconds. The full Palnagotchi default mood set ships
  out of the box (faces are pure ASCII, so no special font setup is needed):

  ```
  (v__v)  (=__=)  (O__O)  ( O_O)  (O_O )  ( 0_0)  (0_0 )
  (+__+)  (-@_@)  (0__0)  (^__^)  (a__a)  (+__+)  (*__*)
  (@__@)  (>__<)  (-__-)  (T_T )  (;__;)  (X__X)  (#__#)
  8====D
  ```

- After the face times out, your normal UI is restored by re-running an
  existing redraw script (default: `draw_display`).
- Won't fire while the voice assistant is listening / thinking / replying —
  the trigger is gated on a configurable condition lambda.

## Requirements

Your device YAML must already define:

| Requirement                             | Default name expected                                    |
|-----------------------------------------|----------------------------------------------------------|
| A `display:` with an `id`               | `m5cores3_lcd` (override via `pwn_display_id`)           |
| A redraw script for the normal UI       | `draw_display` (override via `pwn_redraw_script`)        |
| A condition for "safe to interrupt"     | `id(voice_assistant_phase) == 1` (override via `pwn_show_condition`) |

If you're using
[`stackchan-voice-assistant-base.yaml`](https://github.com/m5stack/esphome-yaml/blob/main/common/stackchan-voice-assistant-base.yaml)
all three are already present and the defaults Just Work.

## Installation

Add it to your device YAML as an ESPHome `package`:

```yaml
packages:
  pwnagotchi:
    url: https://github.com/pfefferle/PwnStack
    file: pwnagotchi-face.yaml
    refresh: 1d
```

Or, if you've cloned this repo locally:

```yaml
packages:
  pwnagotchi: !include path/to/PwnStack/pwnagotchi-face.yaml
```

A complete StackChan example lives in
[`examples/stackchan.yaml`](examples/stackchan.yaml).

## Configuration

Everything is exposed as ESPHome substitutions, so you can override any of
them from your device YAML:

| Substitution              | Default                              | Purpose                                                       |
|---------------------------|--------------------------------------|---------------------------------------------------------------|
| `pwn_display_id`          | `m5cores3_lcd`                       | ID of the existing `display:` to draw onto                    |
| `pwn_redraw_script`       | `draw_display`                       | Script called to restore the normal UI after a face           |
| `pwn_show_condition`      | `id(voice_assistant_phase) == 1`     | Lambda that must return `true` for a face to appear           |
| `pwn_min_delay_ms`        | `30000`                              | Minimum delay between faces (ms)                              |
| `pwn_random_spread_ms`    | `270000`                             | Random spread added on top (ms) — defaults give a 30s..5m gap |
| `pwn_visible_min_ms`      | `2000`                               | Minimum on-screen time per face (ms)                          |
| `pwn_visible_spread_ms`   | `5000`                               | Random spread added on top (ms) — defaults give 2s..7s        |
| `pwn_font_family`         | `Figtree`                            | Google Fonts family for the face text                         |
| `pwn_font_size`           | `64`                                 | Font size (px)                                                |
| `pwn_text_color_r/g/b`    | `0` / `255` / `0`                    | Face color (defaults to classic green-on-black)               |

Example — slower cadence, longer dwell, amber color:

```yaml
substitutions:
  pwn_min_delay_ms: "120000"       # at least 2 minutes between faces
  pwn_random_spread_ms: "480000"   # up to 10 minutes
  pwn_visible_min_ms: "4000"
  pwn_visible_spread_ms: "3000"    # 4s..7s on screen
  pwn_text_color_r: "255"
  pwn_text_color_g: "180"
  pwn_text_color_b: "0"

packages:
  pwnagotchi:
    url: https://github.com/pfefferle/PwnStack
    file: pwnagotchi-face.yaml
    refresh: 1d
```

## How it works

The package contributes:

- A `pwn_face` global holding the currently chosen face string.
- A `pwn_font` font (Figtree by default, Latin Core glyphset only) — the
  Palnagotchi face set is pure ASCII, so no Unicode/symbol font fallbacks
  are needed.
- A `pwn_face_page` page added to your existing `display:` via the `!extend`
  keyword — no need to redefine the display.
- A self-rescheduling `pwn_face_loop` script that waits a random amount of
  time, checks the show condition, picks a random face, switches to
  `pwn_face_page`, waits a random amount of time, and then calls your redraw
  script to restore the normal UI.

## Credits

The mood faces come from
[viniciusbo/m5-palnagotchi](https://github.com/viniciusbo/m5-palnagotchi)
(`palnagotchi/mood.cpp` → `palnagotchi_default_moods[]`), MIT licensed by
Vinícius Borriello.

## License

MIT — see [LICENSE](LICENSE).
