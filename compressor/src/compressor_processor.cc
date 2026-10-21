// Copyright 2025
// Stinky Compressor - DSP Processing Implementation
//
// Each block is processed in three passes over chunks of kChunkFrames:
//
//   1. gain computer: detector peak -> level in dB -> static curve  (per sample)
//   2. envelope:      attack / hold / release, makeup smoothing     (recursive)
//   3. gain stage:    dB -> linear, times makeup, applied to audio  (per sample)
//
// Passes 1 and 3 depend only on their own sample and have AVX2 versions; the
// scalar code performs the same operations in the same order. Pass 2 depends
// on the previous sample and stays scalar. The dB conversions use fast_math.h.

#include "compressor_processor.h"

#include <algorithm>
#include <cmath>

#include "fast_math.h"

namespace fast_compressor {

namespace {

constexpr float kMinLinear = 1.5849e-5f;  // -96 dB floor for the detector

// Peak hold before release may begin. 15 ms covers half a cycle down to
// ~33 Hz, so the detector does not chase individual half-cycles of bass and
// crowd rumble. Longer holds would start to slow speech release audibly.
constexpr float kHoldMs = 15.0f;

// Makeup gain smoothing, so parameter moves never step the output.
constexpr float kMakeupSmoothMs = 10.0f;

// Frames per pass. Keeps the per-sample intermediates in small stack buffers;
// the result does not depend on the chunk size.
constexpr size_t kChunkFrames = 64;

// Level and gain conversions through log2 / exp2:
//   20 log10(x) = kDbPerLog2 * log2(x)
//   10^(-g/20)  = 2^(kLog2PerDb * g)
constexpr float kDbPerLog2 = 6.02059991f;
constexpr float kLog2PerDb = -0.166096405f;

inline float DbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }

// Per-sample step fraction for a one-pole smoother with the given time constant.
inline float StepFraction(float time_ms, double sample_rate) {
  const float samples = time_ms * 0.001f * static_cast<float>(sample_rate);
  if (samples <= 1.0f) return 1.0f;
  return 1.0f - std::exp(-1.0f / samples);
}

// Static curve: gain change in dB (<= 0) for a steady input at level_db.
// slope is 1/ratio - 1 (<= 0).
inline float StaticCurveDb(float level_db, float threshold, float knee,
                           float slope) {
  const float over = level_db - threshold;

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

// Pass 1: peak of the detector channels, level in dB, static curve. Writes
// the gain reduction each sample asks for, in positive dB.
void GainComputer(const float* det_left, const float* det_right,
                  float* target_gr, size_t n, float threshold, float knee,
                  float slope) {
  size_t i = 0;
#if COMPRESSOR_FAST_MATH_AVX2
  const __m256 sign = _mm256_set1_ps(-0.0f);
  const __m256 zero = _mm256_setzero_ps();
  const __m256 floor = _mm256_set1_ps(kMinLinear);
  const __m256 db_per_log2 = _mm256_set1_ps(kDbPerLog2);
  const __m256 v_threshold = _mm256_set1_ps(threshold);
  const __m256 v_slope = _mm256_set1_ps(slope);
  const __m256 v_knee = _mm256_set1_ps(knee);
  const __m256 v_neg_knee = _mm256_set1_ps(-knee);
  const __m256 v_half_knee = _mm256_set1_ps(knee * 0.5f);
  const __m256 v_two_knee = _mm256_set1_ps(2.0f * knee);
  for (; i + 8 <= n; i += 8) {
    const __m256 l = _mm256_andnot_ps(sign, _mm256_loadu_ps(det_left + i));
    const __m256 r = _mm256_andnot_ps(sign, _mm256_loadu_ps(det_right + i));
    const __m256 peak = _mm256_max_ps(_mm256_max_ps(l, r), floor);
    const __m256 level = _mm256_mul_ps(fast_math::FastLog2(peak), db_per_log2);
    const __m256 over = _mm256_sub_ps(level, v_threshold);
    const __m256 hard = _mm256_mul_ps(v_slope, over);
    __m256 gain;
    if (knee > 0.0f) {
      const __m256 two_over = _mm256_add_ps(over, over);
      const __m256 t = _mm256_add_ps(over, v_half_knee);
      const __m256 soft = _mm256_div_ps(
          _mm256_mul_ps(_mm256_mul_ps(v_slope, t), t), v_two_knee);
      const __m256 below = _mm256_cmp_ps(two_over, v_neg_knee, _CMP_LE_OQ);
      const __m256 inside = _mm256_cmp_ps(two_over, v_knee, _CMP_LT_OQ);
      gain = _mm256_andnot_ps(below, _mm256_blendv_ps(hard, soft, inside));
    } else {
      gain = _mm256_and_ps(_mm256_cmp_ps(over, zero, _CMP_GT_OQ), hard);
    }
    _mm256_storeu_ps(target_gr + i, _mm256_xor_ps(gain, sign));
  }
#endif
  for (; i < n; ++i) {
    const float peak = std::max(
        std::max(std::abs(det_left[i]), std::abs(det_right[i])), kMinLinear);
    const float level = fast_math::FastLog2(peak) * kDbPerLog2;
    target_gr[i] = -StaticCurveDb(level, threshold, knee, slope);
  }
}

// Pass 3: smoothed gain reduction (positive dB) and makeup (linear) to one
// linear gain, applied to both channels. left and right may alias.
void ApplyGain(float* left, float* right, const float* gr_db,
               const float* makeup, size_t n) {
  size_t i = 0;
#if COMPRESSOR_FAST_MATH_AVX2
  const __m256 log2_per_db = _mm256_set1_ps(kLog2PerDb);
  for (; i + 8 <= n; i += 8) {
    const __m256 gain = _mm256_mul_ps(
        fast_math::FastExp2(
            _mm256_mul_ps(_mm256_loadu_ps(gr_db + i), log2_per_db)),
        _mm256_loadu_ps(makeup + i));
    const __m256 l = _mm256_loadu_ps(left + i);
    const __m256 r = _mm256_loadu_ps(right + i);
    _mm256_storeu_ps(left + i, _mm256_mul_ps(l, gain));
    _mm256_storeu_ps(right + i, _mm256_mul_ps(r, gain));
  }
#endif
  for (; i < n; ++i) {
    const float gain = fast_math::FastExp2(gr_db[i] * kLog2PerDb) * makeup[i];
    const float l = left[i];
    const float r = right[i];
    left[i] = l * gain;
    right[i] = r * gain;
  }
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
  return StaticCurveDb(input_level_db, params_.threshold_db, params_.knee_db,
                       1.0f / params_.ratio - 1.0f);
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
  const float slope = 1.0f / params_.ratio - 1.0f;  // <= 0

  block_max_gr_db_ = 0.0f;

  float target_gr[kChunkFrames];
  float gr_db[kChunkFrames];
  float makeup[kChunkFrames];

  for (size_t start = 0; start < num_frames; start += kChunkFrames) {
    const size_t n = std::min(kChunkFrames, num_frames - start);

    // The detector reads this chunk before the gain stage writes it, so a
    // sidechain that aliases the main input is fine.
    GainComputer(det_left + start, det_right + start, target_gr, n,
                 params_.threshold_db, params_.knee_db, slope);

    // Smoothing with hold. Attack whenever the target is at or above the
    // envelope; otherwise hold, then release.
    for (size_t i = 0; i < n; ++i) {
      const float target = target_gr[i];
      if (target >= gr_env_db_) {
        gr_env_db_ += attack_k_ * (target - gr_env_db_);
        hold_counter_ = hold_samples_;
      } else if (hold_counter_ > 0) {
        --hold_counter_;
      } else {
        gr_env_db_ += release_k_ * (target - gr_env_db_);
      }
      gr_env_db_ = std::max(gr_env_db_, 0.0f);
      block_max_gr_db_ = std::max(block_max_gr_db_, gr_env_db_);

      // Makeup, smoothed in the linear domain
      makeup_smooth_lin_ +=
          makeup_k_ * (makeup_target_lin_ - makeup_smooth_lin_);

      gr_db[i] = gr_env_db_;
      makeup[i] = makeup_smooth_lin_;
    }

    ApplyGain(left + start, right + start, gr_db, makeup, n);
  }

  gain_reduction_db_ = std::min(-block_max_gr_db_, 0.0f);
}

}  // namespace fast_compressor
