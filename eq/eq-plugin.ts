// Copyright 2025
// Auto-generated TypeScript definitions for EQ plugin

export interface IAudioPluginParam {
  id: string;
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
  
  return [
    {
      id: `band${bandNum}Type`,
      description: `Band ${bandNum} Type`,
      label: `Band ${bandNum} Type`,
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
      id: `band${bandNum}Freq`,
      description: `Band ${bandNum} Frequency`,
      label: `Band ${bandNum} Frequency`,
      min: 0.0,
      max: 1.0,
      defaultValue: frequencyToNormalized(defaultFreqs[bandIndex]),
      getDisplayValue: normalizedToFrequency,
      type: 'float'
    },
    {
      id: `band${bandNum}Gain`,
      description: `Band ${bandNum} Gain`,
      label: `Band ${bandNum} Gain`,
      min: 0.0,
      max: 1.0,
      defaultValue: gainToNormalized(0.0),
      getDisplayValue: normalizedToGain,
      type: 'float'
    },
    {
      id: `band${bandNum}Q`,
      description: `Band ${bandNum} Q`,
      label: `Band ${bandNum} Q`,
      min: 0.0,
      max: 1.0,
      defaultValue: qToNormalized(defaultQs[bandIndex]),
      getDisplayValue: normalizedToQ,
      type: 'float'
    },
    {
      id: `band${bandNum}Enable`,
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
    ...createBandParams(0),
    ...createBandParams(1),
    ...createBandParams(2),
    ...createBandParams(3),
    {
      id: 'outputGain',
      description: 'Output Gain',
      label: 'Output Gain',
      min: 0.0,
      max: 1.0,
      defaultValue: outputGainToNormalized(0.0),
      getDisplayValue: normalizedToOutputGain,
      type: 'float'
    },
    {
      id: 'bypass',
      description: 'Bypass',
      label: 'Bypass',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.0,
      type: 'bool'
    }
  ]
};
