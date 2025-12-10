// Copyright 2025
// Delay Processor Header

#ifndef DELAY_PROCESSOR_H_
#define DELAY_PROCESSOR_H_

#include <cstdint>
#include <vector>

namespace stinky_delay {

struct DelayParams {
  float delay_time_ms = 500.0f;  // 0 to 2000 ms
  float feedback = 0.5f;          // 0.0 to 1.0
  float mix = 0.5f;               // 0.0 to 1.0 (dry/wet)
  float stereo_offset_ms = 0.0f;  // -500 to 500 ms
  bool sync_to_tempo = false;
};

class DelayProcessor {
 public:
  DelayProcessor();
  ~DelayProcessor() = default;

  void Initialize(double sample_rate);
  void Reset();
  void SetParams(const DelayParams& params);
  void ProcessStereo(float* left, float* right, uint32_t frames);

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
