// Copyright 2025
// VST2 Plugin Wrapper Implementation

#include "compressor_vst.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace fast_compressor {

namespace {

constexpr float kThresholdMin = -60.0f;
constexpr float kThresholdMax = 0.0f;
constexpr float kRatioMin = 1.0f;
constexpr float kRatioMax = 20.0f;
constexpr float kAttackMin = 0.1f;
constexpr float kAttackMax = 100.0f;
constexpr float kReleaseMin = 10.0f;
constexpr float kReleaseMax = 1000.0f;
constexpr float kKneeMin = 0.0f;
constexpr float kKneeMax = 12.0f;
constexpr float kMakeupMin = -12.0f;
constexpr float kMakeupMax = 24.0f;

}  // namespace

CompressorVst::CompressorVst(audioMasterCallback audio_master)
    : AudioEffectX(audio_master, 1, kNumParams) {
  setNumInputs(2);
  setNumOutputs(2);
  setUniqueID('FCmp');
  canProcessReplacing();
  
  // Initialize parameters to defaults
  parameters_[kParamThreshold] = 0.5f;
  parameters_[kParamRatio] = 0.3f;
  parameters_[kParamAttack] = 0.2f;
  parameters_[kParamRelease] = 0.3f;
  parameters_[kParamKnee] = 0.0f;
  parameters_[kParamMakeupGain] = 0.5f;
  
  UpdateProcessorParams();
}

void CompressorVst::processReplacing(float** inputs, float** outputs,
                                     VstInt32 sample_frames) {
  float* in_left = inputs[0];
  float* in_right = inputs[1];
  float* out_left = outputs[0];
  float* out_right = outputs[1];
  
  // Copy input to output
  std::memcpy(out_left, in_left, sample_frames * sizeof(float));
  std::memcpy(out_right, in_right, sample_frames * sizeof(float));
  
  // Process
  processor_.ProcessStereo(out_left, out_right, sample_frames);
}

void CompressorVst::processDoubleReplacing(double** inputs, double** outputs,
                                          VstInt32 sample_frames) {
  // Convert double to float, process, and convert back
  float* in_left = inputs[0];
  float* in_right = inputs[1];
  float* out_left = outputs[0];
  float* out_right = outputs[1];
  
  // Simple casting - in production you'd want proper conversion
  processReplacing(reinterpret_cast<float**>(inputs),
                  reinterpret_cast<float**>(outputs),
                  sample_frames);
}

void CompressorVst::setParameter(VstInt32 index, float value) {
  if (index < 0 || index >= kNumParams) return;
  
  parameters_[index] = std::clamp(value, 0.0f, 1.0f);
  UpdateProcessorParams();
}

float CompressorVst::getParameter(VstInt32 index) {
  if (index < 0 || index >= kNumParams) return 0.0f;
  return parameters_[index];
}

void CompressorVst::getParameterLabel(VstInt32 index, char* label) {
  switch (index) {
    case kParamThreshold:
    case kParamKnee:
    case kParamMakeupGain:
      std::strcpy(label, "dB");
      break;
    case kParamRatio:
      std::strcpy(label, ":1");
      break;
    case kParamAttack:
    case kParamRelease:
      std::strcpy(label, "ms");
      break;
    default:
      label[0] = '\0';
  }
}

void CompressorVst::getParameterDisplay(VstInt32 index, char* text) {
  const CompressorParams& params = processor_.GetParams();
  
  switch (index) {
    case kParamThreshold:
      std::sprintf(text, "%.1f", params.threshold_db);
      break;
    case kParamRatio:
      std::sprintf(text, "%.1f", params.ratio);
      break;
    case kParamAttack:
      std::sprintf(text, "%.1f", params.attack_ms);
      break;
    case kParamRelease:
      std::sprintf(text, "%.1f", params.release_ms);
      break;
    case kParamKnee:
      std::sprintf(text, "%.1f", params.knee_db);
      break;
    case kParamMakeupGain:
      std::sprintf(text, "%.1f", params.makeup_gain_db);
      break;
    default:
      text[0] = '\0';
  }
}

void CompressorVst::getParameterName(VstInt32 index, char* text) {
  switch (index) {
    case kParamThreshold:
      std::strcpy(text, "Threshold");
      break;
    case kParamRatio:
      std::strcpy(text, "Ratio");
      break;
    case kParamAttack:
      std::strcpy(text, "Attack");
      break;
    case kParamRelease:
      std::strcpy(text, "Release");
      break;
    case kParamKnee:
      std::strcpy(text, "Knee");
      break;
    case kParamMakeupGain:
      std::strcpy(text, "Makeup Gain");
      break;
    default:
      text[0] = '\0';
  }
}

bool CompressorVst::getEffectName(char* name) {
  std::strcpy(name, "FastCompressor");
  return true;
}

bool CompressorVst::getVendorString(char* text) {
  std::strcpy(text, "FastCompressor");
  return true;
}

bool CompressorVst::getProductString(char* text) {
  std::strcpy(text, "Fast Audio Compressor");
  return true;
}

VstInt32 CompressorVst::getVendorVersion() {
  return 1000;  // Version 1.0.0
}

VstInt32 CompressorVst::canDo(char* text) {
  if (std::strcmp(text, "plugAsChannelInsert") == 0) return 1;
  if (std::strcmp(text, "plugAsSend") == 0) return 1;
  if (std::strcmp(text, "receiveVstEvents") == 0) return -1;
  if (std::strcmp(text, "receiveVstMidiEvent") == 0) return -1;
  if (std::strcmp(text, "receiveVstTimeInfo") == 0) return -1;
  return 0;
}

void CompressorVst::suspend() {
  processor_.Reset();
}

void CompressorVst::resume() {
  processor_.Initialize(getSampleRate());
}

void CompressorVst::setSampleRate(float sample_rate) {
  AudioEffectX::setSampleRate(sample_rate);
  processor_.Initialize(sample_rate);
}

void CompressorVst::UpdateProcessorParams() {
  CompressorParams params;
  
  // Convert normalized parameters to actual values
  params.threshold_db = ParamToDb(parameters_[kParamThreshold], 
                                  kThresholdMin, kThresholdMax);
  
  params.ratio = kRatioMin + parameters_[kParamRatio] * 
                 (kRatioMax - kRatioMin);
  
  params.attack_ms = kAttackMin + parameters_[kParamAttack] * 
                     (kAttackMax - kAttackMin);
  
  params.release_ms = kReleaseMin + parameters_[kParamRelease] * 
                      (kReleaseMax - kReleaseMin);
  
  params.knee_db = kKneeMin + parameters_[kParamKnee] * 
                   (kKneeMax - kKneeMin);
  
  params.makeup_gain_db = ParamToDb(parameters_[kParamMakeupGain], 
                                    kMakeupMin, kMakeupMax);
  
  processor_.SetParams(params);
}

float CompressorVst::ParamToDb(float param, float min_db, float max_db) const {
  return min_db + param * (max_db - min_db);
}

float CompressorVst::DbToParam(float db, float min_db, float max_db) const {
  return (db - min_db) / (max_db - min_db);
}

}  // namespace fast_compressor

// VST2 entry point
AudioEffect* createEffectInstance(audioMasterCallback audio_master) {
  return new fast_compressor::CompressorVst(audio_master);
}
