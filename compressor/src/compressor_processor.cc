// Copyright 2025
// Fast Audio Compressor - DSP Processing Implementation

#include "compressor_processor.h"

#include <algorithm>
#include <cmath>

#include "simd_utils.h"

namespace fast_compressor {

namespace {

constexpr float kMinDb = -96.0f;
constexpr float kEpsilon = 1e-8f;

inline float DbToLinear(float db) {
  return std::pow(10.0f, db / 20.0f);
}

inline float LinearToDb(float linear) {
  return 20.0f * std::log10f(std::max(linear, kEpsilon));
}

}  // namespace

CompressorProcessor::CompressorProcessor()
    : sample_rate_(44100.0),
      envelope_gain_(1.0f),
      gain_reduction_db_(0.0f),
      attack_coeff_(0.0f),
      release_coeff_(0.0f) {}

void CompressorProcessor::Initialize(double sample_rate) {
  sample_rate_ = sample_rate;
  Reset();
  
  // Recalculate coefficients
  SetParams(params_);
}

void CompressorProcessor::SetParams(const CompressorParams& params) {
  params_ = params;
  
  // Calculate attack and release coefficients
  attack_coeff_ = std::exp(-1.0f / (params_.attack_ms * 0.001f * 
                                     static_cast<float>(sample_rate_)));
  release_coeff_ = std::exp(-1.0f / (params_.release_ms * 0.001f * 
                                      static_cast<float>(sample_rate_)));
}

void CompressorProcessor::Reset() {
  envelope_gain_ = 1.0f;
  gain_reduction_db_ = 0.0f;
}

float CompressorProcessor::CalculateGainReduction(float input_level_db) const {
  const float threshold = params_.threshold_db;
  const float ratio = params_.ratio;
  const float knee = params_.knee_db;
  
  if (knee > 0.0f && input_level_db > (threshold - knee / 2.0f) &&
      input_level_db < (threshold + knee / 2.0f)) {
    // Soft knee
    const float knee_factor = (input_level_db - threshold + knee / 2.0f) / knee;
    const float overshoot = input_level_db - threshold;
    return overshoot * (1.0f / ratio - 1.0f) * knee_factor;
  } else if (input_level_db > threshold) {
    // Above threshold
    const float overshoot = input_level_db - threshold;
    return overshoot * (1.0f / ratio - 1.0f);
  }
  
  return 0.0f;  // Below threshold
}

float CompressorProcessor::ApplyEnvelope(float target_gain, 
                                         float current_gain) {
  const float coeff = (target_gain < current_gain) ? 
                      attack_coeff_ : release_coeff_;
  return coeff * current_gain + (1.0f - coeff) * target_gain;
}

void CompressorProcessor::Process(float* buffer, size_t num_frames) {
  const float makeup_gain = DbToLinear(params_.makeup_gain_db);
  
  for (size_t i = 0; i < num_frames * 2; i += 2) {
    // Get peak level from stereo pair
    const float left = std::abs(buffer[i]);
    const float right = std::abs(buffer[i + 1]);
    const float peak = std::max(left, right);
    
    // Convert to dB
    const float peak_db = LinearToDb(peak);
    
    // Calculate target gain reduction
    const float gain_reduction = CalculateGainReduction(peak_db);
    const float target_gain = DbToLinear(gain_reduction);
    
    // Apply envelope
    envelope_gain_ = ApplyEnvelope(target_gain, envelope_gain_);
    
    // Store gain reduction for metering
    gain_reduction_db_ = LinearToDb(envelope_gain_);
    
    // Apply gain and makeup
    const float final_gain = envelope_gain_ * makeup_gain;
    buffer[i] *= final_gain;
    buffer[i + 1] *= final_gain;
  }
}

void CompressorProcessor::ProcessStereo(float* left, float* right, 
                                        size_t num_frames) {
  const float makeup_gain = DbToLinear(params_.makeup_gain_db);
  
  for (size_t i = 0; i < num_frames; ++i) {
    // Get peak level from stereo pair
    const float left_abs = std::abs(left[i]);
    const float right_abs = std::abs(right[i]);
    const float peak = std::max(left_abs, right_abs);
    
    // Convert to dB
    const float peak_db = LinearToDb(peak);
    
    // Calculate target gain reduction
    const float gain_reduction = CalculateGainReduction(peak_db);
    const float target_gain = DbToLinear(gain_reduction);
    
    // Apply envelope
    envelope_gain_ = ApplyEnvelope(target_gain, envelope_gain_);
    
    // Store gain reduction for metering
    gain_reduction_db_ = LinearToDb(envelope_gain_);
    
    // Apply gain and makeup
    const float final_gain = envelope_gain_ * makeup_gain;
    left[i] *= final_gain;
    right[i] *= final_gain;
  }
}

}  // namespace fast_compressor
