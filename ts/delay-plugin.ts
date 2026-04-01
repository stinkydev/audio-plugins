// Copyright Stinky Computing 2026
// Auto-generated TypeScript definitions for Delay plugin

import { IAudioPlugin } from "./audio-plugin";

// Conversion functions from normalized [0,1] to actual values
function normalizedToDelayTime(norm: number): number {
  return 0 + norm * (1000 - 0);
}

function normalizedToMix(norm: number): number {
  return 0 + norm * (1 - 0);
}

// Inverse conversion functions from display value to normalized [0,1]
function delayTimeToNormalized(value: number): number {
  return Math.max(0, Math.min(1, (value - 0) / (1000 - 0)));
}

function mixToNormalized(value: number): number {
  const actual = value / 100;
  return Math.max(0, Math.min(1, (actual - 0) / (1 - 0)));
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
  description: 'Stereo delay',
  inputPorts: [
    { id: 0, name: 'Audio Input', channels: 2, isMain: true },
  ],
  outputPorts: [
    { id: 0, name: 'Audio Output', channels: 2, isMain: true },
  ],
  params: [
    {
      name: 'delayTime',
      id: 0,
      description: 'Delay Time',
      label: 'Delay Time',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.000000,
      getDisplayValue: normalizedToDelayTime,
      getDisplayText: delayTimeToText,
      parseEditValue: delayTimeToNormalized,
      type: 'float'
    },
    {
      name: 'mix',
      id: 1,
      description: 'Mix',
      label: 'Mix',
      min: 0.0,
      max: 1.0,
      defaultValue: 1.000000,
      getDisplayValue: normalizedToMix,
      getDisplayText: mixToText,
      parseEditValue: mixToNormalized,
      type: 'float'
    }
  ]
};
