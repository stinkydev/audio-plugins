// Copyright 2025
// Delay Processor Implementation

#include "delay_processor.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace stinky_delay {

namespace {
constexpr float kMaxDelayMs = 2000.0f;
constexpr double kBufferSeconds = 2.5;  // headroom above the maximum delay
}  // namespace

DelayProcessor::DelayProcessor()
    : sample_rate_(44100.0),
      write_pos_(0),
      max_delay_samples_(0),
      delay_samples_left_(0),
      delay_samples_right_(0),
      initialized_(false) {}

void DelayProcessor::Initialize(double sample_rate) {
  if (sample_rate <= 0.0) sample_rate = 44100.0;
  sample_rate_ = sample_rate;

  max_delay_samples_ = static_cast<uint32_t>(sample_rate * kBufferSeconds);

  // assign() rather than resize() so a re-activation never replays audio
  // left in the buffer from the previous run.
  delay_buffer_left_.assign(max_delay_samples_, 0.0f);
  delay_buffer_right_.assign(max_delay_samples_, 0.0f);

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
  params_.delay_time_ms = std::clamp(params_.delay_time_ms, 0.0f, kMaxDelayMs);
  params_.mix = std::clamp(params_.mix, 0.0f, 1.0f);
  UpdateDelayTimes();
}

void DelayProcessor::UpdateDelayTimes() {
  if (!initialized_) return;

  // Round to the nearest sample so e.g. 33.3333 ms at 48 kHz is exactly 1600
  // samples rather than 1599.
  const long rounded = std::lround(
      static_cast<double>(params_.delay_time_ms) * 0.001 * sample_rate_);
  uint32_t delay_samples = static_cast<uint32_t>(std::max(rounded, 0L));

  // The read position must stay strictly behind the write position after a
  // full wrap, so the largest usable delay is one less than the buffer.
  delay_samples = std::min(delay_samples, max_delay_samples_ - 1);

  delay_samples_left_ = delay_samples;
  delay_samples_right_ = delay_samples;
}

void DelayProcessor::ProcessStereo(float* left, float* right, uint32_t frames) {
  if (!initialized_) return;

  const float dry_gain = 1.0f - params_.mix;
  const float wet_gain = params_.mix;

  for (uint32_t i = 0; i < frames; ++i) {
    // Read both inputs first so left and right may alias for mono use.
    const float in_left = left[i];
    const float in_right = right[i];

    // Write, then read. A delay of zero samples reads back the sample just
    // written, so "0 ms" really is zero and not one sample.
    delay_buffer_left_[write_pos_] = in_left;
    delay_buffer_right_[write_pos_] = in_right;

    const uint32_t read_pos =
        (write_pos_ + max_delay_samples_ - delay_samples_left_) % max_delay_samples_;
    const float delayed_left = delay_buffer_left_[read_pos];
    const float delayed_right = delay_buffer_right_[read_pos];

    left[i] = in_left * dry_gain + delayed_left * wet_gain;
    right[i] = in_right * dry_gain + delayed_right * wet_gain;

    write_pos_ = (write_pos_ + 1) % max_delay_samples_;
  }
}

}  // namespace stinky_delay
