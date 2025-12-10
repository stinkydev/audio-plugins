// Copyright 2025
// Delay Processor Implementation

#include "delay_processor.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace stinky_delay {

DelayProcessor::DelayProcessor()
    : sample_rate_(44100.0),
      write_pos_(0),
      max_delay_samples_(0),
      delay_samples_left_(0),
      delay_samples_right_(0),
      initialized_(false) {}

void DelayProcessor::Initialize(double sample_rate) {
  sample_rate_ = sample_rate;
  
  // Allocate for maximum delay time (2 seconds + stereo offset)
  max_delay_samples_ = static_cast<uint32_t>(sample_rate * 2.5);
  
  delay_buffer_left_.resize(max_delay_samples_, 0.0f);
  delay_buffer_right_.resize(max_delay_samples_, 0.0f);
  
  write_pos_ = 0;
  initialized_ = true;
  
  UpdateDelayTimes();
}

void DelayProcessor::Reset() {
  if (!initialized_) return;
  
  std::fill(delay_buffer_left_.begin(), delay_buffer_left_.end(), 0.0f);
  std::fill(delay_buffer_right_.begin(), delay_buffer_right_.end(), 0.0f);
  write_pos_ = 0;
}

void DelayProcessor::SetParams(const DelayParams& params) {
  params_ = params;
  
  // Clamp parameters
  params_.delay_time_ms = std::clamp(params_.delay_time_ms, 0.0f, 2000.0f);
  params_.mix = std::clamp(params_.mix, 0.0f, 1.0f);
  
  UpdateDelayTimes();
}

void DelayProcessor::UpdateDelayTimes() {
  if (!initialized_) return;
  
  // Calculate delay samples (same for both channels)
  uint32_t delay_samples = static_cast<uint32_t>(
      (params_.delay_time_ms / 1000.0f) * sample_rate_);
  
  // Ensure we don't exceed buffer size
  delay_samples = std::min(delay_samples, max_delay_samples_ - 1);
  
  // Minimum delay of 1 sample
  delay_samples = std::max(1u, delay_samples);
  
  delay_samples_left_ = delay_samples;
  delay_samples_right_ = delay_samples;
}

void DelayProcessor::ProcessStereo(float* left, float* right, uint32_t frames) {
  if (!initialized_) return;
  
  const float dry_gain = 1.0f - params_.mix;
  const float wet_gain = params_.mix;
  
  for (uint32_t i = 0; i < frames; ++i) {
    // Calculate read positions
    uint32_t read_pos = (write_pos_ + max_delay_samples_ - delay_samples_left_) % max_delay_samples_;
    
    // Read delayed samples
    float delayed_left = delay_buffer_left_[read_pos];
    float delayed_right = delay_buffer_right_[read_pos];
    
    // Write input to delay buffer (no feedback)
    delay_buffer_left_[write_pos_] = left[i];
    delay_buffer_right_[write_pos_] = right[i];
    
    // Mix dry and wet signals
    left[i] = left[i] * dry_gain + delayed_left * wet_gain;
    right[i] = right[i] * dry_gain + delayed_right * wet_gain;
    
    // Advance write position
    write_pos_ = (write_pos_ + 1) % max_delay_samples_;
  }
}

}  // namespace stinky_delay
