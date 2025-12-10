// Copyright 2025
// Fast Audio Limiter - DSP Processing Core

#ifndef LIMITER_PROCESSOR_H_
#define LIMITER_PROCESSOR_H_

#include <cstddef>
#include <cstdint>

namespace fast_limiter {

// Limiter parameters
struct LimiterParams {
  float threshold_db = -0.1f;     // Ceiling/threshold in dB
  float output_level_db = -0.1f;  // Target output level in dB
};

// Fast audio limiter with lookahead and SIMD optimization
// A limiter is essentially a compressor with infinite ratio and very fast attack
class LimiterProcessor {
 public:
  LimiterProcessor();
  ~LimiterProcessor();

  // Initialize with sample rate
  void Initialize(double sample_rate);

  // Set limiter parameters
  void SetParams(const LimiterParams& params);
  
  // Get current parameters
  const LimiterParams& GetParams() const { return params_; }

  // Process audio buffer (stereo interleaved)
  void Process(float* buffer, size_t num_frames);

  // Process audio buffer (stereo separate channels)
  void ProcessStereo(float* left, float* right, size_t num_frames);

  // Get current gain reduction in dB
  float GetGainReduction() const { return gain_reduction_db_; }

  // Reset internal state
  void Reset();

 private:
  // Calculate required gain reduction for a given input level
  float CalculateGainReduction(float input_level_db) const;

  // Apply envelope follower with attack/release
  float ApplyEnvelope(float target_gain, float current_gain);
  
  // Update delay buffer and get delayed sample
  void UpdateDelayBuffer(float left_sample, float right_sample);
  void GetDelayedSample(float& left_out, float& right_out);

  LimiterParams params_;
  double sample_rate_;
  float envelope_gain_;
  float gain_reduction_db_;
  float attack_coeff_;
  float release_coeff_;
  
  // Lookahead delay buffer
  float* delay_buffer_left_;
  float* delay_buffer_right_;
  size_t delay_buffer_size_;
  size_t delay_write_pos_;
  size_t delay_read_pos_;
  
  // Auto makeup gain state
  float avg_reduction_db_;
  float alpha_avg_;
};

}  // namespace fast_limiter

#endif  // LIMITER_PROCESSOR_H_
