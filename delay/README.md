# StinkyDelay

A simple stereo delay CLAP plugin.

## Features

- **Delay Time**: 0 to 1000 milliseconds, rounded to the nearest sample.
  0 ms is a true zero-sample delay.
- **Mix**: Dry/wet control (0-100%)
- Stereo processing with synchronized left and right channels
- No latency is reported to the host, so it can be used as an alignment
  (lip-sync) delay without the host compensating it away

## Parameters

All parameters use normalized 0..1 values internally:

- **Delay Time**: Linear mapping (0-1000 ms)
  - `0.0` = 0 ms (default)
  - `0.5` = 500 ms
  - `1.0` = 1000 ms

- **Mix**: Linear mapping (0-100%)
  - `0.0` = 100% dry
  - `0.5` = 50/50 mix
  - `1.0` = 100% wet (default)

## Building

```bash
# From the audio-plugins directory
mkdir build && cd build
cmake ..
cmake --build . --config Release --target StinkyDelay
```

## Testing

```bash
# Run tests
cmake --build . --config Release --target DelayTests
cd delay/Release
./DelayTests
```

## Installation

The plugin will be installed as `StinkyDelay.clap` along with `delay-plugin.ts` TypeScript definitions.

```bash
cmake --install . --prefix /path/to/install --config Release
```

## TypeScript Definitions

The `delay-plugin.ts` file contains complete parameter metadata including:
- Parameter ranges and default values
- Conversion functions from normalized to actual values
- Plugin identification and description

## License

MIT License - see [LICENSE](../LICENSE) file for details.

Copyright (c) 2025 Stinky Computing
