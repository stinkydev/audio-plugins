// Copyright 2025
// Minimal VST2 SDK wrapper - audioeffectx.cpp

#include "../external/aeffectx.h"
#include <cstring>

AudioEffectX::AudioEffectX(audioMasterCallback audio_master, VstInt32 num_programs,
                           VstInt32 num_params)
    : AudioEffect(audio_master, num_programs, num_params) {}

VstIntPtr AudioEffectX::dispatcher(VstInt32 opcode, VstInt32 index, VstIntPtr value,
                                   void* ptr, float opt) {
  switch (opcode) {
    case effGetEffectName:
      if (ptr) getEffectName(static_cast<char*>(ptr));
      return 0;
    case effGetParameterLabel:
      if (ptr) getParameterLabel(index, static_cast<char*>(ptr));
      return 0;
    case effGetParameterDisplay:
      if (ptr) getParameterDisplay(index, static_cast<char*>(ptr));
      return 0;
    case effGetParameterName:
      if (ptr) getParameterName(index, static_cast<char*>(ptr));
      return 0;
    default:
      return AudioEffect::dispatcher(opcode, index, value, ptr, opt);
  }
}

void AudioEffectX::setNumInputs(VstInt32 inputs) {
  cEffect.numInputs = inputs;
}

void AudioEffectX::setNumOutputs(VstInt32 outputs) {
  cEffect.numOutputs = outputs;
}

void AudioEffectX::canProcessReplacing(bool state) {
  if (state) {
    cEffect.flags |= effFlagsCanReplacing;
  } else {
    cEffect.flags &= ~effFlagsCanReplacing;
  }
}

void AudioEffectX::canDoubleReplacing(bool state) {
  if (state) {
    cEffect.flags |= effFlagsCanDoubleReplacing;
  } else {
    cEffect.flags &= ~effFlagsCanDoubleReplacing;
  }
}

void AudioEffectX::setUniqueID(VstInt32 id) {
  cEffect.uniqueID = id;
}

bool AudioEffectX::getEffectName(char*) { return false; }
void AudioEffectX::getParameterLabel(VstInt32, char*) {}
void AudioEffectX::getParameterDisplay(VstInt32, char*) {}
void AudioEffectX::getParameterName(VstInt32, char*) {}
