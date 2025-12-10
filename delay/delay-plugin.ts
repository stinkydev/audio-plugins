// Copyright 2025
// Auto-generated TypeScript definitions for Delay plugin

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

export const DelayPlugin: IAudioPlugin = {
  id: 'com.stinky.delay',
  filename: 'StinkyDelay.clap',
  description: 'Simple stereo delay effect',
  params: [
    {
      id: 'delayTime',
      description: 'Delay Time',
      label: 'Delay Time',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.0,
      getDisplayValue: normalizedToDelayTime,
      type: 'float'
    },
    {
      id: 'mix',
      description: 'Mix',
      label: 'Mix',
      min: 0.0,
      max: 1.0,
      defaultValue: 1.0,
      getDisplayValue: normalizedToMix,
      type: 'float'
    }
  ]
};
