# esphome-components

My collection of [ESPHome](https://esphome.io) external components.

## Components

| Component | Description |
|-----------|-------------|
| [`esphome_avatar`](components/esphome_avatar/) | Header-only Stack-chan-style avatar face for `display:` lambdas — expressions, blink/breath/saccade animation, effect marks, idle mode. |
| [`esphome_roboeyes`](components/esphome_roboeyes/) | [FluxGarage RoboEyes](https://github.com/FluxGarage/RoboEyes) animated robot eyes for `display:` lambdas. **GPL-3.0**, see below. |
| [`esphome_esp32eyes`](components/esphome_esp32eyes/) | [esp32-eyes](https://github.com/playfultechnology/esp32-eyes) Cozmo-style eyes with 18 emotions for `display:` lambdas. **AGPL-3.0**, see below. |

## Usage

Pick the components you want via `external_components:`:

```yaml
external_components:
  - source: github://pfefferle/esphome-components@main
    components: [esphome_avatar]

esphome_avatar:
```

Some components also ship a package partial that does this wiring for
you (see the component's README):

```yaml
packages:
  avatar:
    url: https://github.com/pfefferle/esphome-components
    files: [package.yaml]
    ref: main
```

Each component's README documents its API and configuration. Complete
device configs live in [`examples/`](examples/):

| Example | Device | Face |
|---------|--------|------|
| [`avatar.yaml`](examples/avatar.yaml) | M5Stack Core (320×240) | Stack-chan avatar |
| [`avatar-atoms3.yaml`](examples/avatar-atoms3.yaml) | M5Stack AtomS3 (128×128) | Stack-chan avatar, scaled to fit |
| [`roboeyes-atoms3.yaml`](examples/roboeyes-atoms3.yaml) | M5Stack AtomS3 (128×128) | RoboEyes, every option in Home Assistant |
| [`esp32eyes-atoms3.yaml`](examples/esp32eyes-atoms3.yaml) | M5Stack AtomS3 (128×128) | esp32-eyes, all 18 emotions in Home Assistant |

## License

MIT, see [LICENSE](LICENSE). Individual components may carry additional
upstream attributions in their headers.

Exceptions, because of the upstream libraries they include:

- [`esphome_roboeyes`](components/esphome_roboeyes/) is
  **GPL-3.0-or-later**, see its [LICENSE](components/esphome_roboeyes/LICENSE).
- [`esphome_esp32eyes`](components/esphome_esp32eyes/) is
  **AGPL-3.0-or-later**, see its [LICENSE.AGPL](components/esphome_esp32eyes/LICENSE.AGPL).
  The AGPL also covers use over a network.
