# esphome-components

My collection of [ESPHome](https://esphome.io) external components.

## Components

| Component | Description |
|-----------|-------------|
| [`esphome_avatar`](components/esphome_avatar/) | Header-only Stack-chan-style avatar face for `display:` lambdas — expressions, blink/breath/saccade animation, effect marks, idle mode. |

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

Each component's README documents its API and configuration.

## License

MIT, see [LICENSE](LICENSE). Individual components may carry additional
upstream attributions in their headers.
