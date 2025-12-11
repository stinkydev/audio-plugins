// Copyright 2025
// Auto-generated TypeScript definitions for Delay plugin

import { IAudioPlugin } from "./audio-plugin";

// Conversion functions from normalized [0,1] to actual values
function normalizedToDelayTime(norm: number): number {
  return 0 + norm * (1000 - 0);
}

function normalizedToMix(norm: number): number {
  return 0 + norm * (1 - 0);
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
      id: 100,
      description: 'Delay Time',
      label: 'Delay Time',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.000000,
      getDisplayValue: normalizedToDelayTime,
      getDisplayText: delayTimeToText,
      type: 'float'
    },
    {
      name: 'mix',
      id: 101,
      description: 'Mix',
      label: 'Mix',
      min: 0.0,
      max: 1.0,
      defaultValue: 1.000000,
      getDisplayValue: normalizedToMix,
      getDisplayText: mixToText,
      type: 'float'
    }
  ]
};
