# TypeScript Plugin Definitions

This directory contains auto-generated TypeScript definitions for the audio plugins.

## Generation

The TypeScript files are generated from annotations in the C++ header files. To regenerate:

```bash
npm run generate:ts
```

## Annotations

Each plugin header (`<plugin>/include/<plugin>_clap.h`) contains:

### Plugin Metadata
```cpp
// @ts-plugin-meta
// name: PluginName
// id: com.stinky.plugin
// filename: PluginName.clap
// description: Plugin description
```

### Parameter Annotations
```cpp
kParamIdThreshold = 100,  // @ts-param min=-60.0 max=0.0 default=-20.0 unit=dB label="Threshold"
```

**Supported attributes:**
- `min`, `max` - Parameter range (for float parameters)
- `default` - Default value (in actual units, not normalized)
- `unit` - Display unit (dB, ms, Hz, %, :1)
- `label` - Human-readable label
- `type` - Parameter type (`float`, `bool`, `enum`)
- `scale` - Scaling (`log` for logarithmic, omit for linear)
- `values` - Comma-separated enum values (for enum parameters)

## Single Source of Truth

The C++ headers are the single source of truth. The TypeScript files are generated from them, ensuring:
- Parameter IDs, ranges, defaults, and labels stay in sync
- No manual duplication
- Easy maintenance - just update the C++ annotations

## Usage

```typescript
import { CompressorPlugin, DelayPlugin, EqPlugin, LimiterPlugin } from './plugins';

// Access plugin metadata
console.log(CompressorPlugin.id);  // "com.stinky.compressor"

// Get parameter info
const thresholdParam = CompressorPlugin.params.find(p => p.name === 'threshold');
console.log(thresholdParam.getDisplayText(0.5));  // "-30.0 dB"
```
