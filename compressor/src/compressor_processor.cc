// Copyright 2025
// Stinky Compressor - DSP Processing Implementation

#include "compressor_processor.h"

#include <algorithm>
#include <cmath>

namespace fast_compressor {

namespace {

constexpr float kMinLinear = 1.5849e-5f;  // -96 dB floor for the detector

// Peak hold before release may begin. 15 ms covers half a cycle down to
// ~33 Hz, so the detector does not chase individual half-cycles of bass and
// crowd rumble. Longer holds would start to slow speech release audibly.
constexpr float kHoldMs = 15.0f;

// Makeup gain smoothing, so parameter moves never step the output.
constexpr float kMakeupSmoothMs = 10.0f;

inline float DbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }

inline float LinearToDb(float linear) {
  return 20.0f * std::log10(std::max(linear, kMinLinear));
}

// Per-sample step fraction for a one-pole smoother with the given time constant.
inline float StepFraction(float time_ms, double sample_rate) {
  const float samples = time_ms * 0.001f * static_cast<float>(sample_rate);
  if (samples <= 1.0f) return 1.0f;
  return 1.0f - std::exp(-1.0f / samples);
}

}  // namespace

CompressorProcessor::CompressorProcessor()
    : sample_rate_(44100.0),
      attack_k_(1.0f),
      release_k_(1.0f),
      makeup_k_(1.0f),
      hold_samples_(0),
      gr_env_db_(0.0f),
      hold_counter_(0),
      makeup_target_db_(0.0f),
      makeup_target_lin_(1.0f),
      makeup_smooth_lin_(1.0f),
      block_max_gr_db_(0.0f),
      gain_reduction_db_(0.0f) {
  Initialize(sample_rate_);
}

void CompressorProcessor::Initialize(double sample_rate) {
  if (sample_rate <= 0.0) sample_rate = 44100.0;
  sample_rate_ = sample_rate;
  UpdateCoefficients();
  Reset();
}

void CompressorProcessor::SetParams(const CompressorParams& params) {
  params_ = params;
  params_.ratio = std::max(params_.ratio, 1.0f);
  params_.knee_db = std::max(params_.knee_db, 0.0f);
  UpdateCoefficients();
}

void CompressorProcessor::UpdateCoefficients() {
  attack_k_ = StepFraction(params_.attack_ms, sample_rate_);
  release_k_ = StepFraction(params_.release_ms, sample_rate_);
  makeup_k_ = StepFraction(kMakeupSmoothMs, sample_rate_);
  hold_samples_ = static_cast<size_t>(
      std::lround(kHoldMs * 0.001 * sample_rate_));

  if (params_.auto_makeup) {
    // Compensate half the gain reduction a full-scale steady signal would
    // receive: a fixed function of the curve, so it never drifts or jumps.
    makeup_target_db_ = -0.5f * StaticGainDb(0.0f);
  } else {
    makeup_target_db_ = params_.makeup_gain_db;
  }
  makeup_target_lin_ = DbToLinear(makeup_target_db_);
}

void CompressorProcessor::Reset() {
  gr_env_db_ = 0.0f;
  hold_counter_ = 0;
  makeup_smooth_lin_ = makeup_target_lin_;
  block_max_gr_db_ = 0.0f;
  gain_reduction_db_ = 0.0f;
}

float CompressorProcessor::StaticGainDb(float input_level_db) const {
  const float threshold = params_.threshold_db;
  const float knee = params_.knee_db;
  const float slope = 1.0f / params_.ratio - 1.0f;  // <= 0
  const float over = input_level_db - threshold;

  if (knee > 0.0f) {
    if (2.0f * over <= -knee) return 0.0f;
    if (2.0f * over < knee) {
      // Quadratic knee: continuous in level and slope at both edges
      const float t = over + knee * 0.5f;
      return slope * t * t / (2.0f * knee);
    }
    return slope * over;
  }

  return (over > 0.0f) ? slope * over : 0.0f;
}

void CompressorProcessor::ProcessStereo(float* left, float* right,
                                        size_t num_frames) {
  ProcessStereoWithSidechain(left, right, nullptr, nullptr, num_frames);
}

void CompressorProcessor::ProcessStereoWithSidechain(float* left, float* right,
                                                     const float* sc_left,
                                                     const float* sc_right,
                                                     size_t num_frames) {
  const float* det_left = sc_left ? sc_left : left;
  const float* det_right = sc_right ? sc_right : right;

  block_max_gr_db_ = 0.0f;

  for (size_t i = 0; i < num_frames; ++i) {
    // Read everything before writing: sidechain and channels may alias.
    const float in_left = left[i];
    const float in_right = right[i];
    const float peak = std::max(std::abs(det_left[i]), std::abs(det_right[i]));

    // Gain computer (dB domain, positive = reduction)
    const float level_db = LinearToDb(peak);
    const float target_gr = -StaticGainDb(level_db);

    // Smoothing with hold. Attack whenever the target is at or above the
    // envelope; otherwise hold, then release.
    if (target_gr >= gr_env_db_) {
      gr_env_db_ += attack_k_ * (target_gr - gr_env_db_);
      hold_counter_ = hold_samples_;
    } else if (hold_counter_ > 0) {
      --hold_counter_;
    } else {
      gr_env_db_ += release_k_ * (target_gr - gr_env_db_);
    }
    gr_env_db_ = std::max(gr_env_db_, 0.0f);
    block_max_gr_db_ = std::max(block_max_gr_db_, gr_env_db_);

    // Makeup, smoothed in the linear domain
    makeup_smooth_lin_ += makeup_k_ * (makeup_target_lin_ - makeup_smooth_lin_);

    const float gain = DbToLinear(-gr_env_db_) * makeup_smooth_lin_;
    left[i] = in_left * gain;
    right[i] = in_right * gain;
  }

  gain_reduction_db_ = std::min(-block_max_gr_db_, 0.0f);
}

}  // namespace fast_compressor
