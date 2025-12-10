// Copyright 2025
// Minimal VST2 SDK wrapper - audioeffect.cpp

#include "../external/aeffectx.h"

AudioEffect::AudioEffect(audioMasterCallback audio_master, VstInt32 num_programs,
                         VstInt32 num_params) {
  this->audioMaster = audio_master;
  editor = nullptr;
  
  memset(&cEffect, 0, sizeof(cEffect));
  cEffect.magic = kEffectMagic;
  cEffect.dispatcher = AudioEffect::dispatcherCallback;
  cEffect.process = AudioEffect::processCallback;
  cEffect.setParameter = AudioEffect::setParameterCallback;
  cEffect.getParameter = AudioEffect::getParameterCallback;
  cEffect.numPrograms = num_programs;
  cEffect.numParams = num_params;
  cEffect.numInputs = 0;
  cEffect.numOutputs = 0;
  cEffect.flags = effFlagsCanReplacing;
  cEffect.object = this;
  cEffect.user = nullptr;
  cEffect.uniqueID = 0;
  cEffect.version = 1;
  cEffect.processReplacing = AudioEffect::processReplacingCallback;
  cEffect.processDoubleReplacing = AudioEffect::processDoubleReplacingCallback;
  
  sampleRate = 44100.0f;
}

AudioEffect::~AudioEffect() {}

VstIntPtr VSTCALLBACK AudioEffect::dispatcherCallback(AEffect* effect, VstInt32 opcode,
                                                      VstInt32 index, VstIntPtr value,
                                                      void* ptr, float opt) {
  AudioEffect* ae = static_cast<AudioEffect*>(effect->object);
  return ae ? ae->dispatcher(opcode, index, value, ptr, opt) : 0;
}

void VSTCALLBACK AudioEffect::processCallback(AEffect* effect, float** inputs,
                                              float** outputs, VstInt32 sample_frames) {
  AudioEffect* ae = static_cast<AudioEffect*>(effect->object);
  if (ae) ae->process(inputs, outputs, sample_frames);
}

void VSTCALLBACK AudioEffect::processReplacingCallback(AEffect* effect, float** inputs,
                                                       float** outputs,
                                                       VstInt32 sample_frames) {
  AudioEffect* ae = static_cast<AudioEffect*>(effect->object);
  if (ae) ae->processReplacing(inputs, outputs, sample_frames);
}

void VSTCALLBACK AudioEffect::processDoubleReplacingCallback(AEffect* effect, double** inputs,
                                                             double** outputs,
                                                             VstInt32 sample_frames) {
  AudioEffect* ae = static_cast<AudioEffect*>(effect->object);
  if (ae) ae->processDoubleReplacing(inputs, outputs, sample_frames);
}

void VSTCALLBACK AudioEffect::setParameterCallback(AEffect* effect, VstInt32 index,
                                                   float parameter) {
  AudioEffect* ae = static_cast<AudioEffect*>(effect->object);
  if (ae) ae->setParameter(index, parameter);
}

float VSTCALLBACK AudioEffect::getParameterCallback(AEffect* effect, VstInt32 index) {
  AudioEffect* ae = static_cast<AudioEffect*>(effect->object);
  return ae ? ae->getParameter(index) : 0.0f;
}

VstIntPtr AudioEffect::dispatcher(VstInt32 opcode, VstInt32 index, VstIntPtr value,
                                  void* ptr, float opt) {
  switch (opcode) {
    case effOpen:
      return 0;
    case effClose:
      delete this;
      return 0;
    case effSetSampleRate:
      setSampleRate(opt);
      return 0;
    case effSetBlockSize:
      setBlockSize(value);
      return 0;
    case effMainsChanged:
      if (value) resume();
      else suspend();
      return 0;
    case effGetVendorString:
      if (ptr) getVendorString(static_cast<char*>(ptr));
      return 0;
    case effGetProductString:
      if (ptr) getProductString(static_cast<char*>(ptr));
      return 0;
    case effGetVendorVersion:
      return getVendorVersion();
    case effCanDo:
      return canDo(static_cast<char*>(ptr));
    default:
      return 0;
  }
}

void AudioEffect::process(float**, float**, VstInt32) {}
void AudioEffect::processReplacing(float**, float**, VstInt32) {}
void AudioEffect::processDoubleReplacing(double**, double**, VstInt32) {}
void AudioEffect::setParameter(VstInt32, float) {}
float AudioEffect::getParameter(VstInt32) { return 0.0f; }
void AudioEffect::setSampleRate(float sample_rate) { sampleRate = sample_rate; }
void AudioEffect::setBlockSize(VstInt32 block_size) { blockSize = block_size; }
void AudioEffect::resume() {}
void AudioEffect::suspend() {}
bool AudioEffect::getVendorString(char*) { return false; }
bool AudioEffect::getProductString(char*) { return false; }
VstInt32 AudioEffect::getVendorVersion() { return 0; }
VstInt32 AudioEffect::canDo(char*) { return 0; }
