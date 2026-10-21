// Copyright 2025
// Stinky Limiter - DSP Processing Core
//
// Broadcast-style true-peak brickwall limiter intended for live production
// (ambience and full-mix buses). Design goals, in priority order:
//
//   1. The sample-peak ceiling is never exceeded. This is guaranteed by
//      construction (sliding-minimum + linear ramp over the lookahead), not by
//      tuning, and a final hard clip guards against float rounding.
//   2. True-peak aware. Peaks are detected on a 4x oversampled signal using the
//      ITU-R BS.1770-4 interpolation filter so inter-sample overs are caught.
//      This is a bound, not a guarantee: the gain changes between samples, so
//      the output's BS.1770 true peak can exceed the ceiling by a tested
//      maximum of 0.01 dB. See "The ceiling contract" in the README.
//   3. Stable on sustained loud material (crowd roar) through a dual release:
//      a fast stage for isolated transients and a slow stage that takes over
//      when limiting is sustained.
//   4. Fixed, known latency that the host can compensate.

#ifndef LIMITER_PROCESSOR_H_
#define LIMITER_PROCESSOR_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace fast_limiter {

// Limiter parameters
struct LimiterParams {
  float threshold_db = -1.0f;     // Ceiling (dBTP) applied before output gain
  float output_level_db = -1.0f;  // Final output ceiling (dBTP)
};

class LimiterProcessor {
 public:
  LimiterProcessor();
  ~LimiterProcessor() = default;

  LimiterProcessor(const LimiterProcessor&) = delete;
  LimiterProcessor& operator=(const LimiterProcessor&) = delete;

  // Initialize for a sample rate. Allocates buffers; call from a non-realtime
  // thread (CLAP activate). Resets all state.
  void Initialize(double sample_rate);

  // Set limiter parameters. Realtime safe.
  void SetParams(const LimiterParams& params);

  // Get current parameters
  const LimiterParams& GetParams() const { return params_; }

  // Process audio buffer (stereo interleaved, in place)
  void Process(float* buffer, size_t num_frames);

  // Process audio buffer (stereo separate channels, in place). left and right
  // may alias the same buffer for mono operation.
  void ProcessStereo(float* left, float* right, size_t num_frames);

  // Gain reduction in dB (<= 0) reached during the most recent block.
  float GetGainReduction() const { return gain_reduction_db_; }

  // Total processing latency in samples at the current sample rate.
  uint32_t GetLatencySamples() const {
    return static_cast<uint32_t>(latency_samples_);
  }

  // Lookahead window length in samples.
  size_t GetLookaheadSamples() const { return lookahead_samples_; }

  // Oversampling factor used by the true-peak detector (1, 2 or 4).
  int GetOversamplingFactor() const { return oversampling_; }

  // Reset internal state (clears delay line, envelopes and smoothing).
  void Reset();

 private:
  // Runs the complete limiter for one stereo frame. Returns the delayed,
  // limited output in out_left/out_right.
  void ProcessFrame(float in_left, float in_right, float& out_left,
                    float& out_right);

  // Linked true-peak estimate aligned to input time (n - detector_delay_).
  float DetectPeak() const;

  // Sliding-window minimum over the lookahead window.
  float SlidingMinimum(float value);

  // Moving average over the lookahead window (linear gain ramp).
  float MovingAverage(float value);

  void BeginBlock();
  void EndBlock();

  LimiterParams params_;
  double sample_rate_;

  // Derived from params
  float ceiling_lin_;      // threshold in linear
  float output_gain_;      // output_level - threshold in linear
  float output_ceiling_;   // ceiling_lin_ * output_gain_

  // Derived from sample rate
  int oversampling_;
  size_t detector_delay_;      // true-peak filter delay in input samples
  size_t lookahead_samples_;   // ramp / hold window length (W)
  size_t latency_samples_;     // detector_delay_ + W - 1
  float fast_release_k_;
  float slow_attack_k_;
  float slow_release_k_;

  // Input history for the true-peak interpolation filter
  static constexpr size_t kHistorySize = 16;  // power of two >= taps
  static constexpr size_t kHistoryMask = kHistorySize - 1;
  float history_left_[kHistorySize];
  float history_right_[kHistorySize];
  size_t history_pos_;

  // Lookahead delay line
  std::vector<float> delay_left_;
  std::vector<float> delay_right_;
  size_t delay_mask_;
  size_t delay_pos_;

  // Release envelopes (linear gain, 1.0 = no reduction). Kept in double: in
  // float the one-pole update k * (1 - env) falls below half an ulp of env
  // while still ~1e-3 short of unity, so the gain would never fully release.
  double fast_env_;
  double slow_env_;

  // Sliding minimum (monotonic deque over a ring buffer)
  std::vector<float> min_values_;
  std::vector<uint64_t> min_indices_;
  size_t min_mask_;
  size_t min_head_;
  size_t min_tail_;
  uint64_t sample_index_;

  // Moving average (boxcar) over the lookahead window
  std::vector<float> box_;
  size_t box_pos_;
  double box_sum_;
  size_t box_refresh_counter_;

  // Metering
  float block_min_gain_;
  float gain_reduction_db_;
};

}  // namespace fast_limiter

#endif  // LIMITER_PROCESSOR_H_
