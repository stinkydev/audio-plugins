# StinkyDelay

A high-quality stereo delay CLAP plugin with feedback and stereo offset capabilities.

## Features

- **Delay Time**: 0 to 2000 milliseconds
- **Feedback**: Adjustable feedback amount (0-99%)
- **Mix**: Dry/wet control (0-100%)
- **Stereo Offset**: Create stereo width by offsetting right channel delay (-500 to +500ms)
- **Sync to Tempo**: Option to synchronize delay time to host tempo (future feature)

## Parameters

All parameters use normalized 0..1 values internally:

- **Delay Time**: Linear mapping (0-2000 ms)
  - `0.0` = 0 ms
  - `0.25` = 500 ms
  - `1.0` = 2000 ms

- **Feedback**: Linear mapping (0-99%)
  - `0.0` = 0%
  - `0.5` = 49.5%
  - `1.0` = 99%

- **Mix**: Linear mapping (0-100%)
  - `0.0` = 100% dry
  - `0.5` = 50/50 mix
  - `1.0` = 100% wet

- **Stereo Offset**: Linear mapping (-500 to +500 ms)
  - `0.0` = -500 ms (right earlier)
  - `0.5` = 0 ms (same time)
  - `1.0` = +500 ms (right later)

- **Sync to Tempo**: Boolean (0 = off, 1 = on)

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
