// Copyright 2025
// Delay Processor Header

#ifndef DELAY_PROCESSOR_H_
#define DELAY_PROCESSOR_H_

#include <cstdint>
#include <vector>

namespace stinky_delay {

// Longest delay the processor supports. The CLAP parameter range is derived
// from this so there is a single source of truth.
constexpr float kMaxDelayMs = 1000.0f;

struct DelayParams {
  float delay_time_ms = 0.0f;  // 0 to kMaxDelayMs
  float mix = 1.0f;            // 0.0 to 1.0 (dry/wet)
};

class DelayProcessor {
 public:
  DelayProcessor();
  ~DelayProcessor() = default;

  void Initialize(double sample_rate);
  void Reset();
  void SetParams(const DelayParams& params);
  const DelayParams& GetParams() const { return params_; }

  // In place; left and right may alias the same buffer for mono operation.
  void ProcessStereo(float* left, float* right, uint32_t frames);

  // Current delay in whole samples at the initialized sample rate.
  uint32_t GetDelaySamples() const { return delay_samples_left_; }

 private:
  void UpdateDelayTimes();
  
  double sample_rate_;
  DelayParams params_;
  
  std::vector<float> delay_buffer_left_;
  std::vector<float> delay_buffer_right_;
  
  uint32_t write_pos_;
  uint32_t max_delay_samples_;
  uint32_t delay_samples_left_;
  uint32_t delay_samples_right_;
  
  bool initialized_;
};

}  // namespace stinky_delay

#endif  // DELAY_PROCESSOR_H_
