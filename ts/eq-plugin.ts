// Copyright 2025
// Auto-generated TypeScript definitions for Eq plugin

import { IAudioPlugin } from "./audio-plugin";

// Conversion functions from normalized [0,1] to actual values
function normalizedToBand1Freq(norm: number): number {
  return 20 * Math.pow(20000 / 20, norm);
}

function normalizedToBand1Gain(norm: number): number {
  return -24 + norm * (24 - -24);
}

function normalizedToBand1Q(norm: number): number {
  return 0.1 + norm * (10 - 0.1);
}

function normalizedToBand2Freq(norm: number): number {
  return 20 * Math.pow(20000 / 20, norm);
}

function normalizedToBand2Gain(norm: number): number {
  return -24 + norm * (24 - -24);
}

function normalizedToBand2Q(norm: number): number {
  return 0.1 + norm * (10 - 0.1);
}

function normalizedToBand3Freq(norm: number): number {
  return 20 * Math.pow(20000 / 20, norm);
}

function normalizedToBand3Gain(norm: number): number {
  return -24 + norm * (24 - -24);
}

function normalizedToBand3Q(norm: number): number {
  return 0.1 + norm * (10 - 0.1);
}

function normalizedToBand4Freq(norm: number): number {
  return 20 * Math.pow(20000 / 20, norm);
}

function normalizedToBand4Gain(norm: number): number {
  return -24 + norm * (24 - -24);
}

function normalizedToBand4Q(norm: number): number {
  return 0.1 + norm * (10 - 0.1);
}

function normalizedToOutputGain(norm: number): number {
  return -12 + norm * (12 - -12);
}

// Display text functions with units
function band1FreqToText(norm: number): string {
  const freq = normalizedToBand1Freq(norm);
  return freq >= 1000 ? `${(freq / 1000).toFixed(2)} kHz` : `${freq.toFixed(1)} Hz`;
}

function band1GainToText(norm: number): string {
  return `${normalizedToBand1Gain(norm).toFixed(1)} dB`;
}

function band1QToText(norm: number): string {
  return `${normalizedToBand1Q(norm).toFixed(1)}`;
}

function band2FreqToText(norm: number): string {
  const freq = normalizedToBand2Freq(norm);
  return freq >= 1000 ? `${(freq / 1000).toFixed(2)} kHz` : `${freq.toFixed(1)} Hz`;
}

function band2GainToText(norm: number): string {
  return `${normalizedToBand2Gain(norm).toFixed(1)} dB`;
}

function band2QToText(norm: number): string {
  return `${normalizedToBand2Q(norm).toFixed(1)}`;
}

function band3FreqToText(norm: number): string {
  const freq = normalizedToBand3Freq(norm);
  return freq >= 1000 ? `${(freq / 1000).toFixed(2)} kHz` : `${freq.toFixed(1)} Hz`;
}

function band3GainToText(norm: number): string {
  return `${normalizedToBand3Gain(norm).toFixed(1)} dB`;
}

function band3QToText(norm: number): string {
  return `${normalizedToBand3Q(norm).toFixed(1)}`;
}

function band4FreqToText(norm: number): string {
  const freq = normalizedToBand4Freq(norm);
  return freq >= 1000 ? `${(freq / 1000).toFixed(2)} kHz` : `${freq.toFixed(1)} Hz`;
}

function band4GainToText(norm: number): string {
  return `${normalizedToBand4Gain(norm).toFixed(1)} dB`;
}

function band4QToText(norm: number): string {
  return `${normalizedToBand4Q(norm).toFixed(1)}`;
}

function outputGainToText(norm: number): string {
  return `${normalizedToOutputGain(norm).toFixed(1)} dB`;
}

export const EqPlugin: IAudioPlugin = {
  id: 'com.stinky.eq',
  filename: 'SesameEQ.clap',
  description: 'High-quality 4-band parametric EQ with multiple filter types',
  params: [
    {
      name: 'band1Type',
      id: 100,
      description: 'Band 1 Type',
      label: 'Band 1 Type',
      min: 0.0,
      max: 1.0,
      defaultValue: 1,
      enumValues: [
        { value: 0, label: 'Low Cut' },
        { value: 1, label: 'Bell' },
        { value: 2, label: 'Low Shelf' },
        { value: 3, label: 'High Shelf' },
        { value: 4, label: 'High Cut' },
      ],
      type: 'enum'
    },
    {
      name: 'band1Freq',
      id: 101,
      description: 'Band 1 Frequency',
      label: 'Band 1 Frequency',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.232990,
      getDisplayValue: normalizedToBand1Freq,
      getDisplayText: band1FreqToText,
      type: 'float'
    },
    {
      name: 'band1Gain',
      id: 102,
      description: 'Band 1 Gain',
      label: 'Band 1 Gain',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.500000,
      getDisplayValue: normalizedToBand1Gain,
      getDisplayText: band1GainToText,
      type: 'float'
    },
    {
      name: 'band1Q',
      id: 103,
      description: 'Band 1 Q',
      label: 'Band 1 Q',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.061313,
      getDisplayValue: normalizedToBand1Q,
      getDisplayText: band1QToText,
      type: 'float'
    },
    {
      name: 'band1Enable',
      id: 104,
      description: 'Band 1 Enable',
      label: 'Band 1 Enable',
      min: 0.0,
      max: 1.0,
      defaultValue: 1,
      type: 'bool'
    },
    {
      name: 'band2Type',
      id: 105,
      description: 'Band 2 Type',
      label: 'Band 2 Type',
      min: 0.0,
      max: 1.0,
      defaultValue: 1,
      enumValues: [
        { value: 0, label: 'Low Cut' },
        { value: 1, label: 'Bell' },
        { value: 2, label: 'Low Shelf' },
        { value: 3, label: 'High Shelf' },
        { value: 4, label: 'High Cut' },
      ],
      type: 'enum'
    },
    {
      name: 'band2Freq',
      id: 106,
      description: 'Band 2 Frequency',
      label: 'Band 2 Frequency',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.465980,
      getDisplayValue: normalizedToBand2Freq,
      getDisplayText: band2FreqToText,
      type: 'float'
    },
    {
      name: 'band2Gain',
      id: 107,
      description: 'Band 2 Gain',
      label: 'Band 2 Gain',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.500000,
      getDisplayValue: normalizedToBand2Gain,
      getDisplayText: band2GainToText,
      type: 'float'
    },
    {
      name: 'band2Q',
      id: 108,
      description: 'Band 2 Q',
      label: 'Band 2 Q',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.090909,
      getDisplayValue: normalizedToBand2Q,
      getDisplayText: band2QToText,
      type: 'float'
    },
    {
      name: 'band2Enable',
      id: 109,
      description: 'Band 2 Enable',
      label: 'Band 2 Enable',
      min: 0.0,
      max: 1.0,
      defaultValue: 1,
      type: 'bool'
    },
    {
      name: 'band3Type',
      id: 110,
      description: 'Band 3 Type',
      label: 'Band 3 Type',
      min: 0.0,
      max: 1.0,
      defaultValue: 1,
      enumValues: [
        { value: 0, label: 'Low Cut' },
        { value: 1, label: 'Bell' },
        { value: 2, label: 'Low Shelf' },
        { value: 3, label: 'High Shelf' },
        { value: 4, label: 'High Cut' },
      ],
      type: 'enum'
    },
    {
      name: 'band3Freq',
      id: 111,
      description: 'Band 3 Frequency',
      label: 'Band 3 Frequency',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.666667,
      getDisplayValue: normalizedToBand3Freq,
      getDisplayText: band3FreqToText,
      type: 'float'
    },
    {
      name: 'band3Gain',
      id: 112,
      description: 'Band 3 Gain',
      label: 'Band 3 Gain',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.500000,
      getDisplayValue: normalizedToBand3Gain,
      getDisplayText: band3GainToText,
      type: 'float'
    },
    {
      name: 'band3Q',
      id: 113,
      description: 'Band 3 Q',
      label: 'Band 3 Q',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.090909,
      getDisplayValue: normalizedToBand3Q,
      getDisplayText: band3QToText,
      type: 'float'
    },
    {
      name: 'band3Enable',
      id: 114,
      description: 'Band 3 Enable',
      label: 'Band 3 Enable',
      min: 0.0,
      max: 1.0,
      defaultValue: 1,
      type: 'bool'
    },
    {
      name: 'band4Type',
      id: 115,
      description: 'Band 4 Type',
      label: 'Band 4 Type',
      min: 0.0,
      max: 1.0,
      defaultValue: 3,
      enumValues: [
        { value: 0, label: 'Low Cut' },
        { value: 1, label: 'Bell' },
        { value: 2, label: 'Low Shelf' },
        { value: 3, label: 'High Shelf' },
        { value: 4, label: 'High Cut' },
      ],
      type: 'enum'
    },
    {
      name: 'band4Freq',
      id: 116,
      description: 'Band 4 Frequency',
      label: 'Band 4 Frequency',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.867353,
      getDisplayValue: normalizedToBand4Freq,
      getDisplayText: band4FreqToText,
      type: 'float'
    },
    {
      name: 'band4Gain',
      id: 117,
      description: 'Band 4 Gain',
      label: 'Band 4 Gain',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.500000,
      getDisplayValue: normalizedToBand4Gain,
      getDisplayText: band4GainToText,
      type: 'float'
    },
    {
      name: 'band4Q',
      id: 118,
      description: 'Band 4 Q',
      label: 'Band 4 Q',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.061313,
      getDisplayValue: normalizedToBand4Q,
      getDisplayText: band4QToText,
      type: 'float'
    },
    {
      name: 'band4Enable',
      id: 119,
      description: 'Band 4 Enable',
      label: 'Band 4 Enable',
      min: 0.0,
      max: 1.0,
      defaultValue: 1,
      type: 'bool'
    },
    {
      name: 'outputGain',
      id: 120,
      description: 'Output Gain',
      label: 'Output Gain',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.500000,
      getDisplayValue: normalizedToOutputGain,
      getDisplayText: outputGainToText,
      type: 'float'
    },
    {
      name: 'bypass',
      id: 121,
      description: 'Bypass',
      label: 'Bypass',
      min: 0.0,
      max: 1.0,
      defaultValue: 0,
      type: 'bool'
    }
  ]
};
