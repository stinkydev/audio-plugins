// Copyright 2025
// Auto-generated TypeScript definitions for Delay plugin

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
const DELAY_TIME_MIN = 0.0;
const DELAY_TIME_MAX = 2000.0;
const MIX_MIN = 0.0;
const MIX_MAX = 1.0;

// Conversion functions from normalized [0,1] to actual values
function normalizedToDelayTime(norm: number): number {
  return DELAY_TIME_MIN + norm * (DELAY_TIME_MAX - DELAY_TIME_MIN);
}

function normalizedToMix(norm: number): number {
  return MIX_MIN + norm * (MIX_MAX - MIX_MIN);
}

// Display text functions with units
function delayTimeToText(norm: number): string {
  return `${normalizedToDelayTime(norm).toFixed(1)} ms`;
}

function mixToText(norm: number): string {
  return `${(normalizedToMix(norm) * 100.0).toFixed(1)}%`;
}

export const DelayPlugin: IAudioPlugin = {
  id: 'com.stinky.delay',
  filename: 'StinkyDelay.clap',
  description: 'Simple stereo delay effect',
  params: [
    {
      name: 'delayTime',
      id: 100,  // kParamIdDelayTime
      description: 'Delay Time',
      label: 'Delay Time',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.0,
      getDisplayValue: normalizedToDelayTime,
      getDisplayText: delayTimeToText,
      type: 'float'
    },
    {
      name: 'mix',
      id: 101,  // kParamIdMix
      description: 'Mix',
      label: 'Mix',
      min: 0.0,
      max: 1.0,
      defaultValue: 1.0,
      getDisplayValue: normalizedToMix,
      getDisplayText: mixToText,
      type: 'float'
    }
  ]
};
