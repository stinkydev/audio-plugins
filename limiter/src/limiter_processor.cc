// Copyright 2025
// Fast Audio Limiter - DSP Processing Implementation

#include "limiter_processor.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "simd_utils.h"

namespace fast_limiter {

namespace {

constexpr float kMinDb = -96.0f;
constexpr float kEpsilon = 1e-8f;
constexpr size_t kMaxDelayBufferSize = 48000;  // 1 second at 48kHz

inline float DbToLinear(float db) {
  return std::pow(10.0f, db / 20.0f);
}

inline float LinearToDb(float linear) {
  return 20.0f * ::log10f(std::max(linear, kEpsilon));
}

}  // namespace

LimiterProcessor::LimiterProcessor()
    : sample_rate_(44100.0),
      envelope_gain_(1.0f),
      gain_reduction_db_(0.0f),
      attack_coeff_(0.0f),
      release_coeff_(0.0f),
      delay_buffer_left_(nullptr),
      delay_buffer_right_(nullptr),
      delay_buffer_size_(0),
      delay_write_pos_(0),
      delay_read_pos_(0),
      avg_reduction_db_(0.0f),
      alpha_avg_(0.0f) {
  // Allocate maximum delay buffer
  delay_buffer_left_ = new float[kMaxDelayBufferSize];
  delay_buffer_right_ = new float[kMaxDelayBufferSize];
  std::memset(delay_buffer_left_, 0, kMaxDelayBufferSize * sizeof(float));
  std::memset(delay_buffer_right_, 0, kMaxDelayBufferSize * sizeof(float));
}

LimiterProcessor::~LimiterProcessor() {
  delete[] delay_buffer_left_;
  delete[] delay_buffer_right_;
}

void LimiterProcessor::Initialize(double sample_rate) {
  sample_rate_ = sample_rate;
  Reset();
  
  // Calculate averaging filter coefficient for 2 second time constant
  alpha_avg_ = std::exp(-1.0f / (static_cast<float>(sample_rate) * 2.0f));
  
  // Recalculate coefficients
  SetParams(params_);
}

void LimiterProcessor::SetParams(const LimiterParams& params) {
  params_ = params;
  
  // Brickwall limiter: instant attack (0.1ms) and fast release (50ms)
  constexpr float kAttackMs = 0.1f;
  constexpr float kReleaseMs = 50.0f;
  
  attack_coeff_ = std::exp(-1.0f / (kAttackMs * 0.001f * 
                                     static_cast<float>(sample_rate_)));
  release_coeff_ = std::exp(-1.0f / (kReleaseMs * 0.001f * 
                                      static_cast<float>(sample_rate_)));
  
  // Fixed 5ms lookahead for brickwall limiting
  constexpr float kLookaheadMs = 5.0f;
  size_t new_delay_size = static_cast<size_t>(
      kLookaheadMs * 0.001f * static_cast<float>(sample_rate_));
  new_delay_size = std::min(new_delay_size, kMaxDelayBufferSize - 1);
  
  if (new_delay_size != delay_buffer_size_) {
    delay_buffer_size_ = new_delay_size;
    // Reset positions when size changes
    delay_write_pos_ = 0;
    delay_read_pos_ = 0;
    std::memset(delay_buffer_left_, 0, kMaxDelayBufferSize * sizeof(float));
    std::memset(delay_buffer_right_, 0, kMaxDelayBufferSize * sizeof(float));
  }
}

void LimiterProcessor::Reset() {
  envelope_gain_ = 1.0f;
  gain_reduction_db_ = 0.0f;
  avg_reduction_db_ = 0.0f;
  delay_write_pos_ = 0;
  delay_read_pos_ = 0;
  if (delay_buffer_left_ && delay_buffer_right_) {
    std::memset(delay_buffer_left_, 0, kMaxDelayBufferSize * sizeof(float));
    std::memset(delay_buffer_right_, 0, kMaxDelayBufferSize * sizeof(float));
  }
}

float LimiterProcessor::CalculateGainReduction(float input_level_db) const {
  // Limiter uses infinite ratio - anything above threshold is reduced to threshold
  if (input_level_db > params_.threshold_db) {
    // Gain reduction needed (this will be negative or zero)
    return params_.threshold_db - input_level_db;
  }
  return 0.0f;  // No reduction needed
}

float LimiterProcessor::ApplyEnvelope(float target_gain, float current_gain) {
  // Use attack when gain needs to decrease (limiting action)
  // Use release when gain recovers
  const float coeff = (target_gain < current_gain) ? 
                      attack_coeff_ : release_coeff_;
  return coeff * current_gain + (1.0f - coeff) * target_gain;
}

void LimiterProcessor::UpdateDelayBuffer(float left_sample, float right_sample) {
  if (delay_buffer_size_ == 0) return;
  
  delay_buffer_left_[delay_write_pos_] = left_sample;
  delay_buffer_right_[delay_write_pos_] = right_sample;
  
  delay_write_pos_ = (delay_write_pos_ + 1) % delay_buffer_size_;
}

void LimiterProcessor::GetDelayedSample(float& left_out, float& right_out) {
  if (delay_buffer_size_ == 0) {
    return;  // No delay, keep current values
  }
  
  left_out = delay_buffer_left_[delay_read_pos_];
  right_out = delay_buffer_right_[delay_read_pos_];
  
  delay_read_pos_ = (delay_read_pos_ + 1) % delay_buffer_size_;
}

void LimiterProcessor::Process(float* buffer, size_t num_frames) {
  // Calculate the gain needed to bring threshold to output level
  const float output_gain = DbToLinear(params_.output_level_db - params_.threshold_db);
  
  for (size_t i = 0; i < num_frames * 2; i += 2) {
    float left = buffer[i];
    float right = buffer[i + 1];
    
    // Store in delay buffer for lookahead
    UpdateDelayBuffer(left, right);
    
    // Get peak level from current (future) stereo pair
    const float left_abs = std::abs(left);
    const float right_abs = std::abs(right);
    const float peak = std::max(left_abs, right_abs);
    
    // Convert to dB
    const float peak_db = LinearToDb(peak);
    
    // Calculate target gain reduction in dB (brickwall: anything above threshold gets reduced)
    const float gain_reduction_db = CalculateGainReduction(peak_db);
    const float target_gain = DbToLinear(gain_reduction_db);
    
    // Apply envelope (instant attack, fast release)
    envelope_gain_ = ApplyEnvelope(target_gain, envelope_gain_);
    
    // Store gain reduction for metering
    gain_reduction_db_ = LinearToDb(envelope_gain_);
    
    // Get delayed samples (past audio)
    float delayed_left = left;
    float delayed_right = right;
    GetDelayedSample(delayed_left, delayed_right);
    
    // Apply gain reduction and output scaling to delayed samples
    buffer[i] = delayed_left * envelope_gain_ * output_gain;
    buffer[i + 1] = delayed_right * envelope_gain_ * output_gain;
  }
}

void LimiterProcessor::ProcessStereo(float* left, float* right, 
                                     size_t num_frames) {
  // Calculate the gain needed to bring threshold to output level
  const float output_gain = DbToLinear(params_.output_level_db - params_.threshold_db);
  
  for (size_t i = 0; i < num_frames; ++i) {
    float left_sample = left[i];
    float right_sample = right[i];
    
    // Store in delay buffer for lookahead
    UpdateDelayBuffer(left_sample, right_sample);
    
    // Get peak level from current (future) stereo pair
    const float left_abs = std::abs(left_sample);
    const float right_abs = std::abs(right_sample);
    const float peak = std::max(left_abs, right_abs);
    
    // Convert to dB
    const float peak_db = LinearToDb(peak);
    
    // Calculate target gain reduction in dB (brickwall: anything above threshold gets reduced)
    const float gain_reduction_db = CalculateGainReduction(peak_db);
    const float target_gain = DbToLinear(gain_reduction_db);
    
    // Apply envelope (instant attack, fast release)
    envelope_gain_ = ApplyEnvelope(target_gain, envelope_gain_);
    
    // Store gain reduction for metering (will be negative or zero)
    gain_reduction_db_ = LinearToDb(envelope_gain_);
    
    // Get delayed samples (past audio)
    float delayed_left = left_sample;
    float delayed_right = right_sample;
    GetDelayedSample(delayed_left, delayed_right);
    
    // Apply gain reduction and output scaling to delayed samples
    left[i] = delayed_left * envelope_gain_ * output_gain;
    right[i] = delayed_right * envelope_gain_ * output_gain;
  }
}

}  // namespace fast_limiter
