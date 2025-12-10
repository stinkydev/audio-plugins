// Copyright 2025
// 4-Band EQ - DSP Processing Core

#ifndef EQ_PROCESSOR_H_
#define EQ_PROCESSOR_H_

#include <cstddef>
#include <cstdint>
#include <array>

namespace fast_eq {

// Filter types for each band
enum class FilterType {
  kLowCut = 0,    // Only available on Band 1 (high-pass)
  kBell,          // Available on all bands
  kLowShelf,      // Available on all bands
  kHighShelf,     // Available on all bands
  kHighCut        // Only available on Band 4 (low-pass)
};

// Single band parameters
struct BandParams {
  FilterType type = FilterType::kBell;
  float frequency_hz = 1000.0f;
  float gain_db = 0.0f;
  float q = 0.707f;  // Q factor (bandwidth)
  bool enabled = true;
};

// 4-band EQ parameters
struct EqParams {
  std::array<BandParams, 4> bands;
  float output_gain_db = 0.0f;
  bool bypass = false;
};

// Biquad filter implementation
class BiquadFilter {
 public:
  BiquadFilter();
  
  void SetCoefficients(double b0, double b1, double b2, 
                       double a0, double a1, double a2);
  
  void SetHighCut(double frequency, double q, double sample_rate);
  void SetLowCut(double frequency, double q, double sample_rate);
  void SetLowShelf(double frequency, double gain_db, double q, double sample_rate);
  void SetHighShelf(double frequency, double gain_db, double q, double sample_rate);
  void SetBell(double frequency, double gain_db, double q, double sample_rate);
  
  float Process(float input);
  void Reset();
  
 private:
  double b0_, b1_, b2_;
  double a1_, a2_;
  double x1_, x2_;  // Input delay line
  double y1_, y2_;  // Output delay line
};

// 4-band EQ processor with per-band filter types
class EqProcessor {
 public:
  EqProcessor();
  ~EqProcessor() = default;

  // Initialize with sample rate
  void Initialize(double sample_rate);

  // Set EQ parameters
  void SetParams(const EqParams& params);
  
  // Get current parameters
  const EqParams& GetParams() const { return params_; }

  // Process audio buffer (stereo interleaved)
  void Process(float* buffer, size_t num_frames);

  // Process audio buffer (stereo separate channels)
  void ProcessStereo(float* left, float* right, size_t num_frames);

  // Reset internal state
  void Reset();

 private:
  void UpdateBandCoefficients(size_t band_index);

  EqParams params_;
  double sample_rate_;
  
  // Filters for left and right channels
  std::array<BiquadFilter, 4> filters_left_;
  std::array<BiquadFilter, 4> filters_right_;
};

}  // namespace fast_eq

#endif  // EQ_PROCESSOR_H_
