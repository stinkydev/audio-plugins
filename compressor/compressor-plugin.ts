// Copyright 2025
// Auto-generated TypeScript definitions for Compressor plugin

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
const RATIO_MIN = 1.0;
const RATIO_MAX = 100.0;
const ATTACK_MIN = 0.05;
const ATTACK_MAX = 250.0;
const RELEASE_MIN = 10.0;
const RELEASE_MAX = 2500.0;
const KNEE_MIN = 0.0;
const KNEE_MAX = 12.0;
const MAKEUP_MIN = -12.0;
const MAKEUP_MAX = 24.0;

// Conversion functions from normalized [0,1] to actual values
function normalizedToThreshold(norm: number): number {
  return THRESHOLD_MIN + norm * (THRESHOLD_MAX - THRESHOLD_MIN);
}

function normalizedToRatio(norm: number): number {
  return RATIO_MIN * Math.pow(RATIO_MAX / RATIO_MIN, norm);
}

function normalizedToAttack(norm: number): number {
  return ATTACK_MIN * Math.pow(ATTACK_MAX / ATTACK_MIN, norm);
}

function normalizedToRelease(norm: number): number {
  return RELEASE_MIN * Math.pow(RELEASE_MAX / RELEASE_MIN, norm);
}

function normalizedToKnee(norm: number): number {
  return KNEE_MIN + norm * (KNEE_MAX - KNEE_MIN);
}

function normalizedToMakeup(norm: number): number {
  return MAKEUP_MIN + norm * (MAKEUP_MAX - MAKEUP_MIN);
}

export const CompressorPlugin: IAudioPlugin = {
  id: 'com.stinky.compressor',
  filename: 'StinkyCompressor.clap',
  description: 'High-performance audio compressor with SIMD optimization',
  params: [
    {
      id: 'threshold',
      description: 'Threshold',
      label: 'Threshold',
      min: 0.0,
      max: 1.0,
      defaultValue: ((-20.0 - THRESHOLD_MIN) / (THRESHOLD_MAX - THRESHOLD_MIN)),
      getDisplayValue: normalizedToThreshold,
      type: 'float'
    },
    {
      id: 'ratio',
      description: 'Ratio',
      label: 'Ratio',
      min: 0.0,
      max: 1.0,
      defaultValue: Math.log(4.0 / RATIO_MIN) / Math.log(RATIO_MAX / RATIO_MIN),
      getDisplayValue: normalizedToRatio,
      type: 'float'
    },
    {
      id: 'attack',
      description: 'Attack',
      label: 'Attack',
      min: 0.0,
      max: 1.0,
      defaultValue: Math.log(5.0 / ATTACK_MIN) / Math.log(ATTACK_MAX / ATTACK_MIN),
      getDisplayValue: normalizedToAttack,
      type: 'float'
    },
    {
      id: 'release',
      description: 'Release',
      label: 'Release',
      min: 0.0,
      max: 1.0,
      defaultValue: Math.log(50.0 / RELEASE_MIN) / Math.log(RELEASE_MAX / RELEASE_MIN),
      getDisplayValue: normalizedToRelease,
      type: 'float'
    },
    {
      id: 'knee',
      description: 'Knee',
      label: 'Knee',
      min: 0.0,
      max: 1.0,
      defaultValue: ((0.0 - KNEE_MIN) / (KNEE_MAX - KNEE_MIN)),
      getDisplayValue: normalizedToKnee,
      type: 'float'
    },
    {
      id: 'makeup',
      description: 'Makeup Gain',
      label: 'Makeup Gain',
      min: 0.0,
      max: 1.0,
      defaultValue: ((0.0 - MAKEUP_MIN) / (MAKEUP_MAX - MAKEUP_MIN)),
      getDisplayValue: normalizedToMakeup,
      type: 'float'
    },
    {
      id: 'autoMakeup',
      description: 'Auto Makeup',
      label: 'Auto Makeup',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.0,
      type: 'bool'
    }
  ]
};
