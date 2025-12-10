# Sesame EQ - Interactive Curve Editor

A TypeScript and DOM-based reference implementation for visualizing and editing EQ curves with draggable bands.

## Features

- **Interactive Canvas**: Drag colored dots to adjust frequency and gain in real-time
- **Visual Feedback**: Combined frequency response curve with individual band curves
- **4-Band Parametric EQ**: Each band has independent frequency, gain, and Q controls
- **Logarithmic Frequency Scale**: Natural frequency distribution matching human hearing
- **Responsive Design**: Adapts to different screen sizes
- **Touch Support**: Works on mobile devices
- **Export Settings**: Copy EQ settings as JSON

## Setup

### Prerequisites
- Node.js and npm (for TypeScript compilation)
- A modern web browser
- A local web server (for development)

### Installation

1. Install dependencies:
```bash
cd eq/web-ui
npm install
```

2. Compile TypeScript:
```bash
npm run build
```

3. Start a local server:
```bash
npm run serve
```

4. Open your browser to `http://localhost:8000`

### Development Mode

Watch for changes and auto-compile:
```bash
npm run watch
```

## Usage

### Interactive Controls

- **Drag Band Handles**: Click and drag the colored dots on the canvas to adjust frequency (horizontal) and gain (vertical)
- **Adjust Q**: Use the Q sliders in the control panels to adjust filter bandwidth
- **Activate Band**: Click on a band's control panel to highlight it
- **Reset**: Click "Reset All Bands" to return all bands to flat (0 dB gain)
- **Export**: Click "Export Settings" to copy settings as JSON to clipboard

### Understanding the Display

- **Green Curve**: Combined frequency response of all bands
- **Faded Colored Curves**: Individual band responses
- **Colored Dots**: Draggable band handles (numbered 1-4)
- **Grid Lines**: Frequency (vertical) and gain (horizontal) references
- **0 dB Line**: Emphasized horizontal line showing unity gain

### EQ Parameters

Each band has three parameters:

1. **Frequency** (20 Hz - 20 kHz): Center frequency of the filter
2. **Gain** (-24 dB to +24 dB): Amount of boost or cut
3. **Q** (0.1 - 10.0): Filter bandwidth (higher = narrower)

## Technical Details

### Architecture

- **Pure TypeScript**: Strongly typed for reliability
- **Canvas Rendering**: Hardware-accelerated graphics
- **No Dependencies**: Uses only standard DOM APIs
- **Responsive**: Adapts to high-DPI displays

### EQ Filter Algorithm

The editor uses a peaking EQ filter model:

```
H(ω) = [A²·BW² + (ω² - ω₀²)²] / [BW² + (ω² - ω₀²)²]
```

Where:
- `ω₀`: Center frequency (2π·f₀)
- `BW`: Bandwidth (ω₀/Q)
- `A`: Linear gain (10^(gain_dB/40))

### File Structure

```
web-ui/
├── index.html          # Main HTML page
├── eq-editor.ts        # TypeScript implementation
├── eq-editor.js        # Compiled JavaScript (generated)
├── tsconfig.json       # TypeScript configuration
├── package.json        # npm configuration
└── README.md          # This file
```

## Customization

### Change Number of Bands

Edit the initialization in `eq-editor.ts`:

```typescript
const editor = new EQEditor('eqCanvas', 8); // 8 bands instead of 4
```

### Adjust Frequency Range

Modify the constructor in `EQEditor` class:

```typescript
private minFreq = 10;    // Hz
private maxFreq = 40000; // Hz
```

### Change Colors

Edit the `bandColors` array:

```typescript
private bandColors = [
    '#FF0000', '#00FF00', '#0000FF', '#FFFF00'
];
```

## Integration with Plugin

This reference implementation can be used as a basis for:

1. **Plugin UI**: Create a native UI using the same rendering logic
2. **Web Remote Control**: Embed in a web interface for remote control
3. **Preset Visualization**: Display saved EQ curves
4. **Documentation**: Visual examples for user manual

### Data Format

The EQ settings are exported in this JSON format:

```json
{
  "bands": [
    {
      "frequency": 100,
      "gain": 3.5,
      "q": 1.0,
      "type": "peak",
      "enabled": true
    }
  ]
}
```

## Browser Compatibility

- Chrome/Edge: ✅ Full support
- Firefox: ✅ Full support
- Safari: ✅ Full support
- Mobile browsers: ✅ Touch support included

## License

Copyright 2025 Stinky Computing
