// Copyright 2025
// Audio Plugins TypeScript definitions index

// Re-export individual plugins
export { CompressorPlugin } from './compressor-plugin';
export { EqPlugin } from './eq-plugin';
export { LimiterPlugin } from './limiter-plugin';
export { DelayPlugin } from './delay-plugin';

// Export all plugins as an array
import { CompressorPlugin } from './compressor-plugin';
import { EqPlugin } from './eq-plugin';
import { LimiterPlugin } from './limiter-plugin';
import { DelayPlugin } from './delay-plugin';
import { IAudioPlugin } from './audio-plugin';

export const AllPlugins: IAudioPlugin[] = [
  CompressorPlugin,
  EqPlugin,
  LimiterPlugin,
  DelayPlugin
];
