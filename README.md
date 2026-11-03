# esphome-components

My collection of [ESPHome](https://esphome.io) external components.

## Components

| Component | Description |
|-----------|-------------|
| [`esphome_avatar`](components/esphome_avatar/) | Header-only Stack-chan-style avatar face for `display:` lambdas — expressions, blink/breath/saccade animation, effect marks, idle mode. |
| [`roboeyes`](components/roboeyes/) | [FluxGarage RoboEyes](https://github.com/FluxGarage/RoboEyes) animated robot eyes for `display:` lambdas. **GPL-3.0**, see below. |

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
device configs live in [`examples/`](examples/).

## License

MIT, see [LICENSE](LICENSE). Individual components may carry additional
upstream attributions in their headers.

Exception: [`roboeyes`](components/roboeyes/) is **GPL-3.0-or-later**,
like the upstream RoboEyes library it includes. See its
[LICENSE](components/roboeyes/LICENSE).
