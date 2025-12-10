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
  type: 'float' | 'bool';
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
const FEEDBACK_MIN = 0.0;
const FEEDBACK_MAX = 0.99;
const MIX_MIN = 0.0;
const MIX_MAX = 1.0;
const STEREO_OFFSET_MIN = -500.0;
const STEREO_OFFSET_MAX = 500.0;

// Conversion functions from normalized [0,1] to actual values
function normalizedToDelayTime(norm: number): number {
  return DELAY_TIME_MIN + norm * (DELAY_TIME_MAX - DELAY_TIME_MIN);
}

function normalizedToFeedback(norm: number): number {
  return FEEDBACK_MIN + norm * (FEEDBACK_MAX - FEEDBACK_MIN);
}

function normalizedToMix(norm: number): number {
  return MIX_MIN + norm * (MIX_MAX - MIX_MIN);
}

function normalizedToStereoOffset(norm: number): number {
  return STEREO_OFFSET_MIN + norm * (STEREO_OFFSET_MAX - STEREO_OFFSET_MIN);
}

export const DelayPlugin: IAudioPlugin = {
  id: 'com.stinky.delay',
  filename: 'StinkyDelay.clap',
  description: 'High-quality stereo delay with feedback and stereo offset',
  params: [
    {
      id: 'delayTime',
      description: 'Delay Time',
      label: 'Delay Time',
      min: 0.0,
      max: 1.0,
      defaultValue: ((500.0 - DELAY_TIME_MIN) / (DELAY_TIME_MAX - DELAY_TIME_MIN)),
      getDisplayValue: normalizedToDelayTime,
      type: 'float'
    },
    {
      id: 'feedback',
      description: 'Feedback',
      label: 'Feedback',
      min: 0.0,
      max: 1.0,
      defaultValue: ((0.5 - FEEDBACK_MIN) / (FEEDBACK_MAX - FEEDBACK_MIN)),
      getDisplayValue: normalizedToFeedback,
      type: 'float'
    },
    {
      id: 'mix',
      description: 'Mix',
      label: 'Mix',
      min: 0.0,
      max: 1.0,
      defaultValue: ((0.5 - MIX_MIN) / (MIX_MAX - MIX_MIN)),
      getDisplayValue: normalizedToMix,
      type: 'float'
    },
    {
      id: 'stereoOffset',
      description: 'Stereo Offset',
      label: 'Stereo Offset',
      min: 0.0,
      max: 1.0,
      defaultValue: ((0.0 - STEREO_OFFSET_MIN) / (STEREO_OFFSET_MAX - STEREO_OFFSET_MIN)),
      getDisplayValue: normalizedToStereoOffset,
      type: 'float'
    },
    {
      id: 'syncToTempo',
      description: 'Sync to Tempo',
      label: 'Sync to Tempo',
      min: 0.0,
      max: 1.0,
      defaultValue: 0.0,
      type: 'bool'
    }
  ]
};
