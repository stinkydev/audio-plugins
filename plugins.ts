// Copyright 2025
// Audio Plugins TypeScript definitions index

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

// Re-export individual plugins
export { CompressorPlugin } from './compressor/compressor-plugin';
export { EqPlugin, FilterType } from './eq/eq-plugin';
export { LimiterPlugin } from './limiter/limiter-plugin';

// Export all plugins as an array
import { CompressorPlugin } from './compressor/compressor-plugin';
import { EqPlugin } from './eq/eq-plugin';
import { LimiterPlugin } from './limiter/limiter-plugin';

export const AllPlugins: IAudioPlugin[] = [
  CompressorPlugin,
  EqPlugin,
  LimiterPlugin
];
