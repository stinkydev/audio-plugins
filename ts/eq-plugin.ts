  // Copyright 2025
// Auto-generated TypeScript definitions for EQ plugin

export interface IAudioPluginParam {
  name: string;
  id: number;  // Numeric CLAP parameter ID
  description: string;
  label: string;
  min?: number;
  max?: number;
  defaultValue: number;
  getDisplayValue?: (value: number) => number;
  getDisplayText?: (value: number) => string;
  enumValues?: { value: number; label: string }[];
  type: 'float' | 'bool' | 'enum';
}

export interface IAudioPlugin {
  id: string;
  filename: string;
  description: string;
  params: IAudioPluginParam[];
}

// Parameter ranges (actual units)
const FREQ_MIN = 20.0;
const FREQ_MAX = 20000.0;
const GAIN_MIN = -24.0;
const GAIN_MAX = 24.0;
const Q_MIN = 0.1;
const Q_MAX = 10.0;
const OUTPUT_GAIN_MIN = -12.0;
const OUTPUT_GAIN_MAX = 12.0;

// Filter types
export enum FilterType {
  kLowCut = 0,
  kBell = 1,
  kLowShelf = 2,
  kHighShelf = 3,
  kHighCut = 4
}

// Conversion functions from normalized [0,1] to actual values
function normalizedToFrequency(norm: number): number {
  return FREQ_MIN * Math.pow(FREQ_MAX / FREQ_MIN, norm);
}

function normalizedToGain(norm: number): number {
  return GAIN_MIN + norm * (GAIN_MAX - GAIN_MIN);
}

function normalizedToQ(norm: number): number {
  return Q_MIN + norm * (Q_MAX - Q_MIN);
}

function normalizedToOutputGain(norm: number): number {
  return OUTPUT_GAIN_MIN + norm * (OUTPUT_GAIN_MAX - OUTPUT_GAIN_MIN);
}

// Display text functions with units
function frequencyToText(norm: number): string {
  const freq = normalizedToFrequency(norm);
  if (freq >= 1000.0) {
    return `${(freq / 1000.0).toFixed(2)} kHz`;
  } else {
    return `${freq.toFixed(1)} Hz`;
  }
}

function gainToText(norm: number): string {
  return `${normalizedToGain(norm).toFixed(1)} dB`;
}

function qToText(norm: number): string {
  return normalizedToQ(norm).toFixed(2);
}

function outputGainToText(norm: number): string {
  return `${normalizedToOutputGain(norm).toFixed(1)} dB`;
}

function frequencyToNormalized(hz: number): number {
  return Math.log(hz / FREQ_MIN) / Math.log(FREQ_MAX / FREQ_MIN);
}

function gainToNormalized(db: number): number {
  return (db - GAIN_MIN) / (GAIN_MAX - GAIN_MIN);
}

function qToNormalized(q: number): number {
  return (q - Q_MIN) / (Q_MAX - Q_MIN);
}

function outputGainToNormalized(db: number): number {
  return (db - OUTPUT_GAIN_MIN) / (OUTPUT_GAIN_MAX - OUTPUT_GAIN_MIN);
}

// Helper to get filter type display text
function filterTypeToText(value: number): string {
  const type_names = [
    'Low Cut', 'Bell', 'Low Shelf', 'High Shelf', 'High Cut'
  ];
  const idx = Math.round(value);
  return (idx >= 0 && idx < type_names.length) ? type_names[idx] : 'Unknown';
}

// Helper to create band parameters
function createBandParams(bandIndex: number): IAudioPluginParam[] {
  const bandNum = bandIndex + 1;
  const defaultFreqs = [100, 500, 2000, 8000];
  const defaultTypes = [FilterType.kLowShelf, FilterType.kBell, FilterType.kBell, FilterType.kHighShelf];
  const defaultQs = [0.707, 1.0, 1.0, 0.707];
  
  const baseParamId = 100 + (bandIndex * 5);  // Each band has 5 params: Type, Freq, Gain, Q, Enable
  
  return [
    {
      name: `band${bandNum}Type`,
      id: baseParamId + 0,  // kParamIdBand{N}Type
      description: `Band ${bandNum} Type`,
      label: `B ${bandNum} Type`,
      min: 0,
      max: 4,
      defaultValue: defaultTypes[bandIndex],
      getDisplayText: filterTypeToText,
      enumValues: [
        { value: FilterType.kLowCut, label: 'Low Cut' },
        { value: FilterType.kBell, label: 'Bell' },
        { value: FilterType.kLowShelf, label: 'Low Shelf' },
        { value: FilterType.kHighShelf, label: 'High Shelf' },
        { value: FilterType.kHighCut, label: 'High Cut' }
      ],
      type: 'enum'
    },
    {
      name: `band${bandNum}Freq`,
      id: baseParamId + 1,  // kParamIdBand{N}Freq
      description: `Band ${bandNum} Frequency`,
      label: `B ${bandNum} Frequency`,
      min: 0.0,
      max: 1.0,
      defaultValue: frequencyToNormalized(defaultFreqs[bandIndex]),
      getDisplayValue: normalizedToFrequency,
      getDisplayText: frequencyToText,
      type: 'float'
    },
    {
      name: `band${bandNum}Gain`,
      id: baseParamId + 2,  // kParamIdBand{N}Gain
      description: `Band ${bandNum} Gain`,
      label: `B ${bandNum} Gain`,
      min: 0.0,
      max: 1.0,
      defaultValue: gainToNormalized(0.0),
      getDisplayValue: normalizedToGain,
      getDisplayText: gainToText,
      type: 'float'
    },
    {
      name: `band${bandNum}Q`,
      id: baseParamId + 3,  // kParamIdBand{N}Q
      description: `Band ${bandNum} Q`,
      label: `B ${bandNum} Q`,
      min: 0.0,
      max: 1.0,
      defaultValue: qToNormalized(defaultQs[bandIndex]),
      getDisplayValue: normalizedToQ,
      getDisplayText: qToText,
      type: 'float'
    },
    {
      name: `band${bandNum}Enable`,
      id: baseParamId + 4,  // kParamIdBand{N}Enable
      description: `Band ${bandNum} Enable`,
      label: `Band ${bandNum} Enable`,
      min: 0.0,
      max: 1.0,
      defaultValue: 1.0,
      type: 'bool'
    }
  ];
}

export const EqPlugin: IAudioPlugin = {
  id: 'com.stinky.eq',
  filename: 'StinkyEQ.clap',
  description: 'High-quality 4-band parametric EQ with multiple filter types',
  params: [
    // Bands 1-4 (params 0-19)
    ...createBandParams(0),
    ...createBandParams(1),
    ...createBandParams(2),
    ...createBandParams(3),
    // Output Gain (param 120)
    {
      name: 'outputGain',
      id: 120,  // kParamIdOutputGain
      description: 'Output Gain',
      label: 'Output Gain',
      min: 0.0,
      max: 1.0,
      defaultValue: outputGainToNormalized(0.0),
      getDisplayValue: normalizedToOutputGain,
      getDisplayText: outputGainToText,
      type: 'float'
    },
    // Bypass (param 121)
    {
      name: 'bypass',
      id: 121,  // kParamIdBypass
      description: 'Bypass',
      label: 'Bypass',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.0,
      type: 'bool'
    }
  ]
};
