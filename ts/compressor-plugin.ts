// Copyright Stinky Computing 2026
// Auto-generated TypeScript definitions for Compressor plugin

import { IAudioPlugin } from "./audio-plugin";

// Conversion functions from normalized [0,1] to actual values
function normalizedToThreshold(norm: number): number {
  return -60 + norm * (0 - -60);
}

function normalizedToRatio(norm: number): number {
  return 1 * Math.pow(100 / 1, norm);
}

function normalizedToAttack(norm: number): number {
  return 0.05 * Math.pow(250 / 0.05, norm);
}

function normalizedToRelease(norm: number): number {
  return 10 * Math.pow(2500 / 10, norm);
}

function normalizedToKnee(norm: number): number {
  return 0 + norm * (12 - 0);
}

function normalizedToMakeupGain(norm: number): number {
  return -12 + norm * (24 - -12);
}

// Inverse conversion functions from display value to normalized [0,1]
function thresholdToNormalized(value: number): number {
  return Math.max(0, Math.min(1, (value - -60) / (0 - -60)));
}

function ratioToNormalized(value: number): number {
  return Math.max(0, Math.min(1, Math.log(value / 1) / Math.log(100 / 1)));
}

function attackToNormalized(value: number): number {
  return Math.max(0, Math.min(1, Math.log(value / 0.05) / Math.log(250 / 0.05)));
}

function releaseToNormalized(value: number): number {
  return Math.max(0, Math.min(1, Math.log(value / 10) / Math.log(2500 / 10)));
}

function kneeToNormalized(value: number): number {
  return Math.max(0, Math.min(1, (value - 0) / (12 - 0)));
}

function makeupGainToNormalized(value: number): number {
  return Math.max(0, Math.min(1, (value - -12) / (24 - -12)));
}

// Display text functions with units
function thresholdToText(norm: number): string {
  return `${normalizedToThreshold(norm).toFixed(1)} dB`;
}

function ratioToText(norm: number): string {
  return `${normalizedToRatio(norm).toFixed(1)} :1`;
}

function attackToText(norm: number): string {
  return `${normalizedToAttack(norm).toFixed(1)} ms`;
}

function releaseToText(norm: number): string {
  return `${normalizedToRelease(norm).toFixed(1)} ms`;
}

function kneeToText(norm: number): string {
  return `${normalizedToKnee(norm).toFixed(1)} dB`;
}

function makeupGainToText(norm: number): string {
  return `${normalizedToMakeupGain(norm).toFixed(1)} dB`;
}

export const CompressorPlugin: IAudioPlugin = {
  id: 'com.stinky.compressor',
  filename: 'StinkyCompressor.clap',
  description: 'Compressor',
  inputPorts: [
    { id: 0, name: 'Audio Input', channels: 2, isMain: true },
    { id: 1, name: 'Sidechain Input', channels: 2, isMain: false },
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
      defaultValue: 0.666667,
      getDisplayValue: normalizedToThreshold,
      getDisplayText: thresholdToText,
      parseEditValue: thresholdToNormalized,
      type: 'float'
    },
    {
      name: 'ratio',
      id: 1,
      description: 'Ratio',
      label: 'Ratio',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.301030,
      getDisplayValue: normalizedToRatio,
      getDisplayText: ratioToText,
      parseEditValue: ratioToNormalized,
      type: 'float'
    },
    {
      name: 'attack',
      id: 2,
      description: 'Attack',
      label: 'Attack',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.540691,
      getDisplayValue: normalizedToAttack,
      getDisplayText: attackToText,
      parseEditValue: attackToNormalized,
      type: 'float'
    },
    {
      name: 'release',
      id: 3,
      description: 'Release',
      label: 'Release',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.291488,
      getDisplayValue: normalizedToRelease,
      getDisplayText: releaseToText,
      parseEditValue: releaseToNormalized,
      type: 'float'
    },
    {
      name: 'knee',
      id: 4,
      description: 'Knee',
      label: 'Knee',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.000000,
      getDisplayValue: normalizedToKnee,
      getDisplayText: kneeToText,
      parseEditValue: kneeToNormalized,
      type: 'float'
    },
    {
      name: 'makeupGain',
      id: 5,
      description: 'Makeup Gain',
      label: 'Makeup Gain',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.333333,
      getDisplayValue: normalizedToMakeupGain,
      getDisplayText: makeupGainToText,
      parseEditValue: makeupGainToNormalized,
      type: 'float'
    },
    {
      name: 'autoMakeup',
      id: 6,
      description: 'Auto Makeup',
      label: 'Auto Makeup',
      min: 0.0,
      max: 1.0,
      defaultValue: 0,
      type: 'bool'
    }
  ]
};
