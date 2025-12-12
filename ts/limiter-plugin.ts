// Copyright 2025
// Auto-generated TypeScript definitions for Limiter plugin

import { IAudioPlugin } from "./audio-plugin";

// Conversion functions from normalized [0,1] to actual values
function normalizedToThreshold(norm: number): number {
  return -60 + norm * (0 - -60);
}

function normalizedToOutputLevel(norm: number): number {
  return -60 + norm * (0 - -60);
}

// Display text functions with units
function thresholdToText(norm: number): string {
  return `${normalizedToThreshold(norm).toFixed(1)} dB`;
}

function outputLevelToText(norm: number): string {
  return `${normalizedToOutputLevel(norm).toFixed(1)} dB`;
}

export const LimiterPlugin: IAudioPlugin = {
  id: 'com.stinky.limiter',
  filename: 'StinkyLimiter.clap',
  description: 'High-performance peak limiter with lookahead and SIMD optimization',
  inputPorts: [
    { id: 0, name: 'Audio Input', channels: 2, isMain: true },
  ],
  outputPorts: [
    { id: 0, name: 'Audio Output', channels: 2, isMain: true },
  ],
  params: [
    {
      name: 'threshold',
      id: 0,
      description: 'Threshold',
      label: 'Threshold',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.998333,
      getDisplayValue: normalizedToThreshold,
      getDisplayText: thresholdToText,
      type: 'float'
    },
    {
      name: 'outputLevel',
      id: 1,
      description: 'Output Level',
      label: 'Output Level',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.998333,
      getDisplayValue: normalizedToOutputLevel,
      getDisplayText: outputLevelToText,
      type: 'float'
    }
  ]
};
