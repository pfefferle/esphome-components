# esphome_esp32eyes

[esp32-eyes](https://github.com/playfultechnology/esp32-eyes) for ESPHome:
Cozmo-style animated eyes with 18 emotions. The eye shapes change while
they look around and move from one emotion to the next.

All upstream sources (commit `2738dbf`, 2024-03-07) are included unchanged.
Three files make them build on ESPHome:

- `Arduino.h`: `millis()`, `random()`, `min`/`max` for ESP-IDF builds. On
  Arduino builds it hands over to the real `Arduino.h`.
- `U8g2lib.h`: a stand-in for the one u8g2 display class esp32-eyes uses.
  It draws on any ESPHome `display::Display`.
- `esphome_esp32eyes.h`: the wrapper for display lambdas.

## License

**AGPL-3.0-or-later.** The upstream source files say GNU Affero General
Public License v3 or later, see [LICENSE.AGPL](LICENSE.AGPL). The upstream
repository's [LICENSE](LICENSE) file is the plain GPL-3.0 text; it is
included as it is, but the stricter AGPL from the file headers applies.

This is different from the rest of this repository (MIT). Firmware that
includes this component is a combined work under the AGPL, and the AGPL
also covers use over a network: whoever uses the device over the network
(for example through Home Assistant) has to be able to get its source.

esp32-eyes: Copyright (c) 2023 Alastair Aitchison, Playful Technology,
(c) 2020 Luis Llamas (www.luisllamas.es)

## Usage

```yaml
external_components:
  - source: github://pfefferle/esphome-components@main
    components: [esphome_esp32eyes]

esphome_esp32eyes:

display:
  - platform: ...
    update_interval: 33ms   # the frame rate of the eyes
    lambda: |-
      static esp32eyes::Eyes eyes;
      if (eyes.begin(it, 48)) {   // true once, on the first frame; 48 = eye size
        eyes.face().Behavior.SetEmotion(eEmotions::Happy, 1.0);
      }
      eyes.draw(it);
```

`eyes.face()` is the upstream `Face`, so its whole API works:

- `Behavior.GoToEmotion(eEmotions::Sad)` and the other 17 emotions
  (`Normal`, `Angry`, `Glee`, `Happy`, `Sad`, `Worried`, `Focused`,
  `Annoyed`, `Surprised`, `Skeptic`, `Frustrated`, `Unimpressed`,
  `Sleepy`, `Suspicious`, `Squint`, `Furious`, `Scared`, `Awe`)
- `Behavior.SetEmotion(emotion, weight)`: how likely each emotion is when
  `RandomBehavior` picks one on its own
- `RandomBehavior`, `RandomLook`, `RandomBlink`: switch the automatic
  emotion changes, looking around and blinking on or off
- `DoBlink()`, `LookLeft()`, `LookRight()`, `LookTop()`, `LookBottom()`,
  `LookFront()`, `Look.LookAt(x, y)` with x and y from -1 to 1
- `Blink.Timer.SetIntervalMillis(ms)`: time between blinks

The face is created on the first frame, because upstream needs the screen
size for that. Outside the display lambda (buttons, selects), check
`eyes.ready()` before using `eyes.face()`.

Two settings come from the ESPHome side, both can change every frame:

- `eyes.set_colors(background, main)`: any ESPHome colors.
- `eyes.set_scale(1.2)`: eye size. Upstream draws its eyes at a fixed
  40×40px; the scale applies to the whole drawing around the screen
  center, so 1.0 is upstream's size. The `eyeSize` passed to `begin()`
  only sets the distance between the eyes. Large scales can touch the
  screen edge when the eyes look sideways.

[`examples/esp32eyes-atoms3.yaml`](../../examples/esp32eyes-atoms3.yaml)
exposes all of it to Home Assistant, including an RGB light for the eye
color and a slider for the eye size.
