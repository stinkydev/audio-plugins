# Sesame EQ

A high-quality 4-band parametric equalizer CLAP plugin with multiple filter types.

## Features

- **4 Independent Bands**: Each band can be configured independently
- **Multiple Filter Types**:
  - Low Pass: Cuts frequencies above the cutoff
  - High Pass: Cuts frequencies below the cutoff
  - Low Shelf: Boosts or cuts low frequencies
  - High Shelf: Boosts or cuts high frequencies
  - Peak: Boosts or cuts at a specific frequency
  - Band Pass: Allows only frequencies around the center frequency
  - Notch: Cuts frequencies around the center frequency
  - All Pass: Phase-only filter (for special effects)
  
- **Per-Band Controls**:
  - Filter Type selection
  - Frequency (20 Hz - 20 kHz)
  - Gain (-24 dB to +24 dB)
  - Q Factor (0.1 to 10.0)
  - Enable/Disable switch

- **Global Controls**:
  - Output Gain (-12 dB to +12 dB)
  - Bypass switch

## Web UI

The `web-ui` folder contains an interactive TypeScript-based EQ curve editor for visualization and preset creation.

### Using Normalized Parameters

The web editor now supports exporting and importing settings using normalized 0..1 parameter values, matching the plugin's internal representation:

```javascript
// Access the editor instance (available after page load)
const editor = window.eqEditor;

// Export settings as normalized 0..1 values
const normalizedSettings = editor.exportNormalizedSettings();
console.log(normalizedSettings);
// Example output:
// {
//   bands: [
//     { type: 2, frequency: 0.234, gain: 0.5, q: 0.061, enabled: 1.0 },
//     { type: 1, frequency: 0.456, gain: 0.5, q: 0.091, enabled: 1.0 },
//     ...
//   ],
//   outputGain: 0.5,
//   bypass: 0.0
// }

// Import settings from normalized 0..1 values
editor.importNormalizedSettings({
  bands: [
    { type: 2, frequency: 0.3, gain: 0.6, q: 0.08, enabled: 1.0 },
    // ... more bands
  ],
  outputGain: 0.5,
  bypass: 0.0
});
```

### Parameter Ranges (Normalized 0..1)

- **Frequency**: Logarithmic mapping (20 Hz to 20 kHz)
  - `0.0` = 20 Hz
  - `0.5` ≈ 632 Hz
  - `1.0` = 20000 Hz
  
- **Gain**: Linear mapping (-24 dB to +24 dB)
  - `0.0` = -24 dB
  - `0.5` = 0 dB
  - `1.0` = +24 dB
  
- **Q**: Linear mapping (0.1 to 10.0)
  - `0.0` = 0.1
  - `0.091` ≈ 1.0 (default)
  - `1.0` = 10.0
  
- **Output Gain**: Linear mapping (-12 dB to +12 dB)
  - `0.0` = -12 dB
  - `0.5` = 0 dB
  - `1.0` = +12 dB

## Building

### Prerequisites

- CMake 3.20 or higher
- C++20 compatible compiler
- Git (for fetching dependencies)

### Build Instructions

```bash
# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build . --config Release

# Run tests (optional)
ctest -C Release
```

The compiled plugin will be in `build/Release/StinkyEQ.clap` (Windows) or appropriate location for your platform.

## Installation

### Windows
Copy `StinkyEQ.clap` to:
- `C:\Program Files\Common Files\CLAP\`

### macOS
Copy `StinkyEQ.clap` to:
- `/Library/Audio/Plug-Ins/CLAP/`
- `~/Library/Audio/Plug-Ins/CLAP/`

### Linux
Copy `StinkyEQ.clap` to:
- `~/.clap/`
- `/usr/lib/clap/`

## Usage

1. Load the plugin in your CLAP-compatible DAW
2. Configure each band:
   - Select the desired filter type
   - Set the frequency, gain, and Q factor
   - Enable/disable the band as needed
3. Adjust the output gain if needed
4. Use the bypass switch to compare processed vs. unprocessed audio

## Technical Details

- **Sample Rate**: Supports all standard sample rates
- **Latency**: Zero latency (no look-ahead)
- **Processing**: Stereo in-place processing using biquad IIR filters
- **Filter Design**: Based on Robert Bristow-Johnson's Audio EQ Cookbook

## Default Band Configuration

- **Band 1**: Low Shelf at 100 Hz
- **Band 2**: Peak at 500 Hz
- **Band 3**: Peak at 2 kHz
- **Band 4**: High Shelf at 8 kHz

## License

Copyright 2025 Stinky Computing

## Credits

- CLAP format by [Free Audio](https://github.com/free-audio/clap)
- Filter algorithms based on the Audio EQ Cookbook
