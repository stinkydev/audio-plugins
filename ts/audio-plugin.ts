
export interface IAudioPluginParam {
  name: string;
  id: number;  // Numeric CLAP parameter ID
  description: string;
  label: string;
  min?: number;
  max?: number;
  defaultValue: number;
  getDisplayValue?: (value: number) => number;
  getDisplayText?: (value: number) => string;
  enumValues?: { value: number; label: string }[];
  type: 'float' | 'bool' | 'enum';
}

export interface IAudioPort {
  id: number;
  name: string;
  channels: number;
  isMain: boolean;
}

export interface IAudioPlugin {
  id: string;
  filename: string;
  description: string;
  inputPorts?: IAudioPort[];
  outputPorts?: IAudioPort[];
  params: IAudioPluginParam[];
}