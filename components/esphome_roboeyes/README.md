# esphome_roboeyes

[FluxGarage RoboEyes](https://github.com/FluxGarage/RoboEyes) for ESPHome:
smoothly animated robot eyes (rounded rectangles) with moods, blinking,
idle looking around and a few one-shot animations.

`FluxGarage_RoboEyes.h` is the upstream library (v1.1.2), included
unchanged. `esphome_roboeyes.h` is a small adapter that draws it on any
ESPHome `display::Display` and provides the Arduino helpers RoboEyes
needs on ESP-IDF builds.

## License

**GPL-3.0-or-later**, like upstream RoboEyes. See [LICENSE](LICENSE).
This is different from the rest of this repository (MIT): firmware that
includes this component is a combined work under the GPL.

RoboEyes: Copyright (C) 2024-2025 Dennis Hoelscher, www.fluxgarage.com

## Usage

```yaml
external_components:
  - source: github://pfefferle/esphome-components@main
    components: [esphome_roboeyes]

esphome_roboeyes:

display:
  - platform: ...
    update_interval: 33ms   # the frame rate of the eyes
    lambda: |-
      static roboeyes::Face face;
      static bool configured = false;
      if (!configured) {
        auto &eyes = face.eyes();
        eyes.setWidth(44, 44);
        eyes.setHeight(44, 44);
        eyes.setBorderradius(12, 12);
        eyes.setSpacebetween(12);
        eyes.setAutoblinker(roboeyes::ON, 3, 2);
        eyes.setIdleMode(roboeyes::ON, 2, 2);
        configured = true;
      }
      face.draw(it);
```

`face.eyes()` is the RoboEyes object, so the whole upstream API works:
`setMood(roboeyes::HAPPY)` (`DEFAULT`, `TIRED`, `ANGRY`, `HAPPY`),
`setPosition(roboeyes::NE)`, `setCuriosity()`, `setSweat()`,
`anim_laugh()`, `anim_confused()`, `blink()`, and so on.

Set the sizes (`setWidth`, `setHeight`, `setBorderradius`,
`setSpacebetween`) once, as above, not on every frame: they also reset
the eyes' animation targets, so calling them every frame cancels every
blink. `setMood()`, `setAutoblinker()` and `setIdleMode()` only store
values and can be called every frame.

[`examples/roboeyes-atoms3.yaml`](../../examples/roboeyes-atoms3.yaml)
exposes every option to Home Assistant.

Differences to the Arduino library:

- The display's `update_interval` is the frame rate. RoboEyes'
  `update()` and `setFramerate()` are not used, because ESPHome clears the
  screen before every frame.
- On the first frame the eyes are centered for the actual display size
  and the sizes set before it. Upstream assumes a 128×64 screen with
  36px eyes for its start position.
- `face.set_colors(background, main)` takes ESPHome colors instead of
  `setDisplayColors()`.
- The constants (`ON`, `HAPPY`, `N`, …) live in the `roboeyes`
  namespace instead of global macros.
