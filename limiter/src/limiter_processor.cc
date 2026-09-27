// Copyright 2025
// Stinky Limiter - DSP Processing Implementation
//
// Signal flow per frame (both channels linked):
//
//   input ──┬─► delay line (latency L) ──────────────────────────► × gain ─► clip ─► out
//           │
//           └─► true-peak detect ─► target gain g = min(1, ceiling / peak)
//                                    │
//                                    ├─► fast envelope (instant attack, fast release)
//                                    └─► slow envelope (slow attack, slow release)
//                                            │
//                                    r = min(fast, slow)            (r <= g always)
//                                            │
//                                    sliding minimum over W samples (hold)
//                                            │
//                                    moving average over W samples  (linear ramp)
//                                            │
//                                          gain
//
// Why this never exceeds the ceiling: the smoothed gain at output time n is the
// average of W sliding minima whose windows all contain detector time n-W+1, so
// gain[n] <= r[n-W+1] <= g[n-W+1]. The audio sample emitted at time n is the
// one the detector saw at time n-W+1, so |out| <= |x| * ceiling / peak <= ceiling.

#include "limiter_processor.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace fast_limiter {

namespace {

// Timing constants (fixed by design; see README for rationale)
constexpr float kLookaheadMs = 5.0f;
constexpr float kFastReleaseMs = 80.0f;   // isolated transients
constexpr float kSlowAttackMs = 200.0f;   // how quickly sustained limiting is recognised
constexpr float kSlowReleaseMs = 500.0f;  // recovery after sustained limiting

constexpr float kMinDb = -96.0f;
constexpr float kEpsilon = 1e-8f;

// ITU-R BS.1770-4 Annex 2 true-peak interpolation filter: a 48-tap FIR at 4x
// rate expressed as four 12-tap polyphase branches. Applied as
// sum_k c[phase][k] * x[n - k]. The main lobe sits between x[n-6] and x[n-5],
// so the estimate is aligned to input time n-6 (kTruePeakDelay).
constexpr size_t kTruePeakTaps = 12;
constexpr size_t kTruePeakPhases = 4;
constexpr size_t kTruePeakDelay = 6;
constexpr float kTruePeakCoeffs[kTruePeakPhases][kTruePeakTaps] = {
    {0.0017089843750f, 0.0109863281250f, -0.0196533203125f, 0.0332031250000f,
     -0.0594482421875f, 0.1373291015625f, 0.9721679687500f, -0.1022949218750f,
     0.0476074218750f, -0.0266113281250f, 0.0148925781250f, -0.0083007812500f},
    {-0.0291748046875f, 0.0292968750000f, -0.0517578125000f, 0.0891113281250f,
     -0.1665039062500f, 0.4650878906250f, 0.7797851562500f, -0.2003173828125f,
     0.1015625000000f, -0.0582275390625f, 0.0330810546875f, -0.0189208984375f},
    {-0.0189208984375f, 0.0330810546875f, -0.0582275390625f, 0.1015625000000f,
     -0.2003173828125f, 0.7797851562500f, 0.4650878906250f, -0.1665039062500f,
     0.0891113281250f, -0.0517578125000f, 0.0292968750000f, -0.0291748046875f},
    {-0.0083007812500f, 0.0148925781250f, -0.0266113281250f, 0.0476074218750f,
     -0.1022949218750f, 0.9721679687500f, 0.1373291015625f, -0.0594482421875f,
     0.0332031250000f, -0.0196533203125f, 0.0109863281250f, 0.0017089843750f},
};

inline float DbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }

inline float LinearToDb(float linear) {
  return 20.0f * std::log10(std::max(linear, kEpsilon));
}

// Per-sample step fraction for a one-pole smoother with the given time constant.
inline float StepFraction(float time_ms, double sample_rate) {
  const float samples = time_ms * 0.001f * static_cast<float>(sample_rate);
  if (samples <= 1.0f) return 1.0f;
  return 1.0f - std::exp(-1.0f / samples);
}

inline size_t NextPowerOfTwo(size_t n) {
  size_t p = 1;
  while (p < n) p <<= 1;
  return p;
}

}  // namespace

LimiterProcessor::LimiterProcessor()
    : sample_rate_(44100.0),
      ceiling_lin_(1.0f),
      output_gain_(1.0f),
      output_ceiling_(1.0f),
      oversampling_(1),
      detector_delay_(0),
      lookahead_samples_(1),
      latency_samples_(0),
      fast_release_k_(1.0f),
      slow_attack_k_(1.0f),
      slow_release_k_(1.0f),
      history_pos_(0),
      delay_mask_(0),
      delay_pos_(0),
      fast_env_(1.0f),
      slow_env_(1.0f),
      min_mask_(0),
      min_head_(0),
      min_tail_(0),
      sample_index_(0),
      box_pos_(0),
      box_sum_(0.0),
      box_refresh_counter_(0),
      block_min_gain_(1.0f),
      gain_reduction_db_(0.0f) {
  Initialize(sample_rate_);
}

void LimiterProcessor::Initialize(double sample_rate) {
  if (sample_rate <= 0.0) sample_rate = 44100.0;
  sample_rate_ = sample_rate;

  // True-peak oversampling per BS.1770: 4x up to 48 kHz family, 2x for 88.2/96,
  // none above (inter-sample overs are negligible at 176.4 kHz and up).
  if (sample_rate < 64000.0) {
    oversampling_ = 4;
  } else if (sample_rate < 128000.0) {
    oversampling_ = 2;
  } else {
    oversampling_ = 1;
  }
  detector_delay_ = (oversampling_ > 1) ? kTruePeakDelay : 0;

  lookahead_samples_ = static_cast<size_t>(
      std::lround(kLookaheadMs * 0.001 * sample_rate));
  lookahead_samples_ = std::max<size_t>(lookahead_samples_, 1);
  latency_samples_ = detector_delay_ + lookahead_samples_ - 1;

  fast_release_k_ = StepFraction(kFastReleaseMs, sample_rate);
  slow_attack_k_ = StepFraction(kSlowAttackMs, sample_rate);
  slow_release_k_ = StepFraction(kSlowReleaseMs, sample_rate);

  // Buffers
  const size_t delay_size = NextPowerOfTwo(latency_samples_ + 1);
  delay_left_.assign(delay_size, 0.0f);
  delay_right_.assign(delay_size, 0.0f);
  delay_mask_ = delay_size - 1;

  const size_t min_size = NextPowerOfTwo(lookahead_samples_ + 1);
  min_values_.assign(min_size, 1.0f);
  min_indices_.assign(min_size, 0);
  min_mask_ = min_size - 1;

  box_.assign(lookahead_samples_, 1.0f);

  Reset();
  SetParams(params_);
}

void LimiterProcessor::SetParams(const LimiterParams& params) {
  params_ = params;
  ceiling_lin_ = DbToLinear(std::max(params_.threshold_db, kMinDb));
  output_gain_ = DbToLinear(params_.output_level_db - params_.threshold_db);
  output_ceiling_ = ceiling_lin_ * output_gain_;
}

void LimiterProcessor::Reset() {
  std::fill(std::begin(history_left_), std::end(history_left_), 0.0f);
  std::fill(std::begin(history_right_), std::end(history_right_), 0.0f);
  history_pos_ = 0;

  std::fill(delay_left_.begin(), delay_left_.end(), 0.0f);
  std::fill(delay_right_.begin(), delay_right_.end(), 0.0f);
  delay_pos_ = 0;

  fast_env_ = 1.0f;
  slow_env_ = 1.0f;

  min_head_ = 0;
  min_tail_ = 0;
  sample_index_ = 0;

  std::fill(box_.begin(), box_.end(), 1.0f);
  box_pos_ = 0;
  box_sum_ = static_cast<double>(box_.size());
  box_refresh_counter_ = 0;

  block_min_gain_ = 1.0f;
  gain_reduction_db_ = 0.0f;
}

float LimiterProcessor::DetectPeak() const {
  // The raw sample at the detector's alignment point is always included, which
  // is what makes the sample-peak guarantee exact regardless of the filter.
  const size_t aligned = (history_pos_ - detector_delay_) & kHistoryMask;
  float peak = std::max(std::abs(history_left_[aligned]),
                        std::abs(history_right_[aligned]));

  if (oversampling_ > 1) {
    float xl[kTruePeakTaps];
    float xr[kTruePeakTaps];
    for (size_t k = 0; k < kTruePeakTaps; ++k) {
      const size_t idx = (history_pos_ - k) & kHistoryMask;
      xl[k] = history_left_[idx];
      xr[k] = history_right_[idx];
    }

    // 4x uses all phases; 2x uses phases 0 and 2 (the half-sample point).
    const size_t phase_step = (oversampling_ == 4) ? 1 : 2;
    for (size_t p = 0; p < kTruePeakPhases; p += phase_step) {
      const float* c = kTruePeakCoeffs[p];
      float al = 0.0f;
      float ar = 0.0f;
      for (size_t k = 0; k < kTruePeakTaps; ++k) {
        al += c[k] * xl[k];
        ar += c[k] * xr[k];
      }
      peak = std::max(peak, std::max(std::abs(al), std::abs(ar)));
    }
  }
  return peak;
}

float LimiterProcessor::SlidingMinimum(float value) {
  // Monotonic deque: values increase from head to tail. Drop everything at the
  // tail that is >= the new value (it can never be the minimum again), push the
  // new value, then expire the head if it has left the window.
  while (min_tail_ != min_head_) {
    const size_t last = (min_tail_ - 1) & min_mask_;
    if (min_values_[last] < value) break;
    min_tail_ = last;
  }
  min_values_[min_tail_] = value;
  min_indices_[min_tail_] = sample_index_;
  min_tail_ = (min_tail_ + 1) & min_mask_;

  const uint64_t expire_before = sample_index_ + 1 - lookahead_samples_;
  if (sample_index_ + 1 >= lookahead_samples_ &&
      min_indices_[min_head_] < expire_before) {
    min_head_ = (min_head_ + 1) & min_mask_;
  }

  ++sample_index_;
  return min_values_[min_head_];
}

float LimiterProcessor::MovingAverage(float value) {
  const size_t w = box_.size();
  box_sum_ += static_cast<double>(value) - static_cast<double>(box_[box_pos_]);
  box_[box_pos_] = value;
  box_pos_ = (box_pos_ + 1 == w) ? 0 : box_pos_ + 1;

  // Re-sum exactly once per window to keep the running sum drift-free over
  // arbitrarily long sessions. Amortised O(1) per sample.
  if (++box_refresh_counter_ >= w) {
    box_refresh_counter_ = 0;
    double sum = 0.0;
    for (float v : box_) sum += static_cast<double>(v);
    box_sum_ = sum;
  }

  const float avg = static_cast<float>(box_sum_ / static_cast<double>(w));
  return std::min(avg, 1.0f);
}

inline void LimiterProcessor::ProcessFrame(float in_left, float in_right,
                                           float& out_left, float& out_right) {
  // Detector history and delay line
  history_pos_ = (history_pos_ + 1) & kHistoryMask;
  history_left_[history_pos_] = in_left;
  history_right_[history_pos_] = in_right;

  delay_left_[delay_pos_] = in_left;
  delay_right_[delay_pos_] = in_right;
  const size_t read_pos = (delay_pos_ - latency_samples_) & delay_mask_;
  const float delayed_left = delay_left_[read_pos];
  const float delayed_right = delay_right_[read_pos];
  delay_pos_ = (delay_pos_ + 1) & delay_mask_;

  // Target gain for this detector frame
  const float peak = DetectPeak();
  const float target = (peak > ceiling_lin_) ? ceiling_lin_ / peak : 1.0f;

  // Fast envelope: instant attack, fast release. Always <= target.
  if (target < fast_env_) {
    fast_env_ = target;
  } else {
    fast_env_ += fast_release_k_ * (target - fast_env_);
  }

  // Slow envelope: only builds up when limiting is sustained, then holds the
  // gain down smoothly so dense material (crowd noise) does not flutter.
  if (target < slow_env_) {
    slow_env_ += slow_attack_k_ * (target - slow_env_);
  } else {
    slow_env_ += slow_release_k_ * (target - slow_env_);
  }

  const float released = std::min(fast_env_, slow_env_);

  // Hold for the lookahead window, then ramp linearly over it.
  const float held = SlidingMinimum(released);
  const float gain = MovingAverage(held);

  block_min_gain_ = std::min(block_min_gain_, gain);

  // Apply to the delayed audio, scale to the output level, and clip as a guard
  // against floating-point rounding.
  const float g = gain * output_gain_;
  out_left = std::clamp(delayed_left * g, -output_ceiling_, output_ceiling_);
  out_right = std::clamp(delayed_right * g, -output_ceiling_, output_ceiling_);
}

void LimiterProcessor::BeginBlock() { block_min_gain_ = 1.0f; }

void LimiterProcessor::EndBlock() {
  gain_reduction_db_ = std::min(LinearToDb(block_min_gain_), 0.0f);
}

void LimiterProcessor::Process(float* buffer, size_t num_frames) {
  BeginBlock();
  for (size_t i = 0; i < num_frames; ++i) {
    float* frame = buffer + 2 * i;
    ProcessFrame(frame[0], frame[1], frame[0], frame[1]);
  }
  EndBlock();
}

void LimiterProcessor::ProcessStereo(float* left, float* right,
                                     size_t num_frames) {
  BeginBlock();
  for (size_t i = 0; i < num_frames; ++i) {
    // Read both inputs before writing so left and right may alias.
    const float in_left = left[i];
    const float in_right = right[i];
    float out_left;
    float out_right;
    ProcessFrame(in_left, in_right, out_left, out_right);
    left[i] = out_left;
    right[i] = out_right;
  }
  EndBlock();
}

}  // namespace fast_limiter
