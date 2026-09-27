// Copyright 2025
// Stinky Compressor - DSP Processing Core
//
// Feed-forward compressor for broadcast ambience and commentary buses.
//
//   - Static curve with a proper quadratic soft knee (Giannoulis et al. 2012).
//   - Linked peak detection on the sidechain (or the main input).
//   - Gain reduction is smoothed in the dB domain with the attack / release
//     controls, plus a fixed hold so the detector does not release between the
//     half-cycles of low-frequency content. That is what keeps crowd rumble and
//     PA bass from turning into intermodulation distortion.
//   - Deterministic auto makeup derived from the static curve (no adaptation,
//     no level creep), and smoothed makeup changes so parameter moves don't click.

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

class CompressorProcessor {
 public:
  CompressorProcessor();
  ~CompressorProcessor() = default;

  // Initialize with sample rate. Resets all state.
  void Initialize(double sample_rate);

  // Set compressor parameters. Realtime safe.
  void SetParams(const CompressorParams& params);

  // Get current parameters
  const CompressorParams& GetParams() const { return params_; }

  // Process audio buffer (stereo separate channels, in place). left and right
  // may alias the same buffer for mono operation.
  void ProcessStereo(float* left, float* right, size_t num_frames);

  // Process audio buffer with sidechain input. If sidechain pointers are null,
  // the main input is used for detection.
  void ProcessStereoWithSidechain(float* left, float* right,
                                  const float* sc_left, const float* sc_right,
                                  size_t num_frames);

  // Deepest gain reduction reached in the most recent block, in dB (<= 0).
  float GetGainReduction() const { return gain_reduction_db_; }

  // Static gain computer: gain change in dB (<= 0) applied to a steady input
  // at the given level. Exposed for testing and metering.
  float StaticGainDb(float input_level_db) const;

  // Makeup gain currently targeted, in dB (manual or auto).
  float MakeupGainDb() const { return makeup_target_db_; }

  // Reset internal state
  void Reset();

 private:
  void UpdateCoefficients();

  CompressorParams params_;
  double sample_rate_;

  // Smoothing coefficients (per-sample step fractions)
  float attack_k_;
  float release_k_;
  float makeup_k_;
  size_t hold_samples_;

  // Detector state
  float gr_env_db_;       // smoothed gain reduction, positive dB
  size_t hold_counter_;   // samples of hold remaining before release may start

  // Makeup state
  float makeup_target_db_;
  float makeup_target_lin_;
  float makeup_smooth_lin_;

  // Metering
  float block_max_gr_db_;
  float gain_reduction_db_;
};

}  // namespace fast_compressor

#endif  // COMPRESSOR_PROCESSOR_H_
