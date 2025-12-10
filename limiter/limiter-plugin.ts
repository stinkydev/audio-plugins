// Copyright 2025
// Auto-generated TypeScript definitions for Limiter plugin

export interface IAudioPluginParam {
  id: string;
  description: string;
  label: string;
  min?: number;
  max?: number;
  defaultValue: number;
  getDisplayValue?: (value: number) => number;
  type: 'float' | 'bool';
}

export interface IAudioPlugin {
  id: string;
  filename: string;
  description: string;
  params: IAudioPluginParam[];
}

// Parameter ranges (actual units)
const THRESHOLD_MIN = -60.0;
const THRESHOLD_MAX = 0.0;
const OUTPUT_LEVEL_MIN = -60.0;
const OUTPUT_LEVEL_MAX = 0.0;

// Conversion functions from normalized [0,1] to actual values
function normalizedToThreshold(norm: number): number {
  return THRESHOLD_MIN + norm * (THRESHOLD_MAX - THRESHOLD_MIN);
}

function normalizedToOutputLevel(norm: number): number {
  return OUTPUT_LEVEL_MIN + norm * (OUTPUT_LEVEL_MAX - OUTPUT_LEVEL_MIN);
}

export const LimiterPlugin: IAudioPlugin = {
  id: 'com.stinky.limiter',
  filename: 'StinkyLimiter.clap',
  description: 'High-performance peak limiter with lookahead and SIMD optimization',
  params: [
    {
      id: 'threshold',
      description: 'Threshold',
      label: 'Threshold',
      min: 0.0,
      max: 1.0,
      defaultValue: ((-0.1 - THRESHOLD_MIN) / (THRESHOLD_MAX - THRESHOLD_MIN)),
      getDisplayValue: normalizedToThreshold,
      type: 'float'
    },
    {
      id: 'outputLevel',
      description: 'Output Level',
      label: 'Output Level',
      min: 0.0,
      max: 1.0,
      defaultValue: ((-0.1 - OUTPUT_LEVEL_MIN) / (OUTPUT_LEVEL_MAX - OUTPUT_LEVEL_MIN)),
      getDisplayValue: normalizedToOutputLevel,
      type: 'float'
    }
  ]
};
