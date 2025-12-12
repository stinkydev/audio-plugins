// Copyright 2025
// Fast Audio Compressor - DSP Processing Core

#ifndef COMPRESSOR_PROCESSOR_H_
#define COMPRESSOR_PROCESSOR_H_

#include <cstddef>
#include <cstdint>

namespace fast_compressor {

// Compressor parameters
struct CompressorParams {
  float threshold_db = -20.0f;  // Threshold in dB
  float ratio = 4.0f;            // Compression ratio (1:1 to inf:1)
  float attack_ms = 5.0f;        // Attack time in milliseconds
  float release_ms = 50.0f;      // Release time in milliseconds
  float knee_db = 0.0f;          // Knee width in dB (0 = hard knee)
  float makeup_gain_db = 0.0f;   // Output makeup gain in dB
  bool auto_makeup = false;      // Auto makeup gain enabled
};

// Fast audio compressor with SIMD optimization
class CompressorProcessor {
 public:
  CompressorProcessor();
  ~CompressorProcessor() = default;

  // Initialize with sample rate
  void Initialize(double sample_rate);

  // Set compressor parameters
  void SetParams(const CompressorParams& params);
  
  // Get current parameters
  const CompressorParams& GetParams() const { return params_; }

  // Process audio buffer (stereo separate channels)
  void ProcessStereo(float* left, float* right, size_t num_frames);

  // Process audio buffer (stereo separate channels) with sidechain input
  // If sidechain pointers are null, uses main input for detection
  void ProcessStereoWithSidechain(float* left, float* right, 
                                  const float* sc_left, const float* sc_right,
                                  size_t num_frames);

  // Get current gain reduction in dB
  float GetGainReduction() const { return gain_reduction_db_; }

  // Reset internal state
  void Reset();

 private:
  // Calculate gain reduction for a given input level
  float CalculateGainReduction(float input_level_db) const;

  // Apply envelope follower
  float ApplyEnvelope(float target_gain, float current_gain);

  CompressorParams params_;
  double sample_rate_;
  float envelope_gain_;
  float gain_reduction_db_;
  float attack_coeff_;
  float release_coeff_;
  
  // Auto makeup gain state
  float c_dev_;  // Average deviation of gain reduction
  float alpha_avg_;  // Averaging filter coefficient (2 second time constant)
};

}  // namespace fast_compressor

#endif  // COMPRESSOR_PROCESSOR_H_
