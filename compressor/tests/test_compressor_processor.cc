// Copyright 2025
// Unit tests for CompressorProcessor

#include "compressor_processor.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace fast_compressor {
namespace {

constexpr double kSampleRate = 44100.0;
constexpr float kEpsilon = 1e-5f;
constexpr double kPi = 3.14159265358979323846;

class CompressorProcessorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    processor_.Initialize(kSampleRate);
  }

  CompressorProcessor processor_;
};

TEST_F(CompressorProcessorTest, InitializeSetsSampleRate) {
  processor_.Initialize(48000.0);
  // Verify initialization doesn't crash
  SUCCEED();
}

TEST_F(CompressorProcessorTest, ProcessSilenceRemainsUnchanged) {
  std::vector<float> left(512, 0.0f);
  std::vector<float> right(512, 0.0f);

  processor_.ProcessStereo(left.data(), right.data(), 512);

  for (size_t i = 0; i < left.size(); ++i) {
    EXPECT_FLOAT_EQ(left[i], 0.0f);
    EXPECT_FLOAT_EQ(right[i], 0.0f);
  }
}

TEST_F(CompressorProcessorTest, ProcessBelowThresholdUncompressed) {
  CompressorParams params;
  params.threshold_db = -20.0f;
  params.ratio = 4.0f;
  params.attack_ms = 1.0f;
  params.release_ms = 100.0f;
  params.makeup_gain_db = 0.0f;
  processor_.SetParams(params);

  // Generate signal below threshold (-30 dB = 0.0316)
  std::vector<float> left(512, 0.0316f);
  std::vector<float> right(512, 0.0316f);

  processor_.ProcessStereo(left.data(), right.data(), 512);

  // Signal below threshold should pass relatively unchanged
  for (size_t i = 0; i < left.size(); ++i) {
    EXPECT_NEAR(left[i], 0.0316f, 0.005f);
    EXPECT_NEAR(right[i], 0.0316f, 0.005f);
  }
}

TEST_F(CompressorProcessorTest, ProcessAboveThresholdCompresses) {
  CompressorParams params;
  params.threshold_db = -20.0f;
  params.ratio = 4.0f;
  params.attack_ms = 1.0f;
  params.release_ms = 100.0f;
  params.makeup_gain_db = 0.0f;
  processor_.SetParams(params);

  // Generate signal above threshold (-10 dB = 0.316)
  std::vector<float> left(512, 0.316f);
  std::vector<float> right(512, 0.316f);

  processor_.ProcessStereo(left.data(), right.data(), 512);

  // Signal should be compressed (reduced)
  for (size_t i = 100; i < left.size(); ++i) {  // Skip attack phase
    EXPECT_LT(std::abs(left[i]), 0.316f);
    EXPECT_LT(std::abs(right[i]), 0.316f);
  }
}

TEST_F(CompressorProcessorTest, MakeupGainIncreasesOutput) {
  CompressorParams params;
  params.threshold_db = -20.0f;
  params.ratio = 4.0f;
  params.attack_ms = 1.0f;
  params.release_ms = 100.0f;
  params.makeup_gain_db = 12.0f;  // +12 dB makeup
  processor_.SetParams(params);

  std::vector<float> left(512, 0.316f);
  std::vector<float> right(512, 0.316f);

  processor_.ProcessStereo(left.data(), right.data(), 512);

  // With makeup gain, output should be higher than without
  bool has_makeup = false;
  for (size_t i = 100; i < left.size(); ++i) {
    if (std::abs(left[i]) > 0.316f || std::abs(right[i]) > 0.316f) {
      has_makeup = true;
      break;
    }
  }
  EXPECT_TRUE(has_makeup);
}

// Reference static curve from Giannoulis, Massberg & Reiss (2012)
float ReferenceGainDb(float x, float threshold, float ratio, float knee) {
  const float slope = 1.0f / ratio - 1.0f;
  const float over = x - threshold;
  if (knee > 0.0f && 2.0f * std::abs(over) <= knee) {
    const float t = over + knee / 2.0f;
    return slope * t * t / (2.0f * knee);
  }
  return over > 0.0f ? slope * over : 0.0f;
}

TEST_F(CompressorProcessorTest, StaticCurveMatchesReference) {
  for (float knee : {0.0f, 3.0f, 6.0f, 12.0f}) {
    for (float ratio : {1.0f, 2.0f, 4.0f, 20.0f, 100.0f}) {
      CompressorParams params;
      params.threshold_db = -20.0f;
      params.ratio = ratio;
      params.knee_db = knee;
      processor_.SetParams(params);
      for (float x = -60.0f; x <= 0.0f; x += 0.5f) {
        const float expected = ReferenceGainDb(x, -20.0f, ratio, knee);
        EXPECT_NEAR(processor_.StaticGainDb(x), expected, 1e-4f)
            << "knee " << knee << " ratio " << ratio << " level " << x;
      }
    }
  }
}

TEST_F(CompressorProcessorTest, SoftKneeNeverBoostsAndIsContinuous) {
  CompressorParams params;
  params.threshold_db = -20.0f;
  params.ratio = 4.0f;
  params.knee_db = 12.0f;
  processor_.SetParams(params);

  float previous = 0.0f;
  for (float x = -40.0f; x <= 0.0f; x += 0.1f) {
    const float g = processor_.StaticGainDb(x);
    EXPECT_LE(g, 1e-6f) << "gain above unity at " << x << " dB";
    EXPECT_LE(g, previous + 1e-4f) << "gain not monotonic at " << x << " dB";
    previous = g;
  }
  // Knee edges match the hard-knee line
  EXPECT_NEAR(processor_.StaticGainDb(-26.0f), 0.0f, 1e-5f);
  EXPECT_NEAR(processor_.StaticGainDb(-14.0f), -4.5f, 1e-4f);
  // Midpoint of the knee has half the slope's effect of the upper edge
  EXPECT_NEAR(processor_.StaticGainDb(-20.0f), -1.125f, 1e-4f);
}

TEST_F(CompressorProcessorTest, SteadyStateFollowsStaticCurveThroughKnee) {
  CompressorParams params;
  params.threshold_db = -20.0f;
  params.ratio = 4.0f;
  params.knee_db = 12.0f;
  params.attack_ms = 1.0f;
  params.release_ms = 10.0f;
  processor_.SetParams(params);

  for (float level_db = -30.0f; level_db <= -8.0f; level_db += 2.0f) {
    processor_.Reset();
    const float amp = std::pow(10.0f, level_db / 20.0f);
    std::vector<float> left(4410, amp), right(4410, amp);
    processor_.ProcessStereo(left.data(), right.data(), left.size());
    const float out_db = 20.0f * std::log10(std::abs(left.back()));
    EXPECT_NEAR(out_db - level_db, processor_.StaticGainDb(level_db), 0.02f)
        << "at " << level_db << " dB";
  }
}

TEST_F(CompressorProcessorTest, LowFrequencyDistortionIsNegligible) {
  // 50 Hz at -6 dBFS, 14 dB over threshold, default attack / release. A
  // detector that releases between half-cycles produces several percent THD;
  // the hold keeps the gain steady across the cycle.
  CompressorParams params;
  params.threshold_db = -20.0f;
  params.ratio = 4.0f;
  params.attack_ms = 5.0f;
  params.release_ms = 50.0f;
  processor_.SetParams(params);

  const double freq = 50.0;
  const size_t n = static_cast<size_t>(kSampleRate) * 2;
  std::vector<float> left(n), right(n);
  for (size_t i = 0; i < n; ++i) {
    left[i] = right[i] = 0.5f * static_cast<float>(std::sin(2.0 * kPi * freq * i / kSampleRate));
  }
  processor_.ProcessStereo(left.data(), right.data(), n);

  // DFT of the last second at the fundamental and harmonics 2..10
  const size_t start = n / 2;
  const size_t len = n - start;
  double fundamental = 0.0, harmonics = 0.0;
  for (int h = 1; h <= 10; ++h) {
    double re = 0.0, im = 0.0;
    for (size_t i = 0; i < len; ++i) {
      const double phase = 2.0 * kPi * freq * h * (start + i) / kSampleRate;
      re += left[start + i] * std::cos(phase);
      im += left[start + i] * std::sin(phase);
    }
    const double mag = std::sqrt(re * re + im * im);
    if (h == 1) fundamental = mag; else harmonics += mag * mag;
  }
  const double thd_percent = 100.0 * std::sqrt(harmonics) / fundamental;
  EXPECT_LT(thd_percent, 0.1);

  // and it is still compressing: -6 dB in, about -10.5 dB of reduction
  EXPECT_LT(processor_.GetGainReduction(), -9.0f);
}

TEST_F(CompressorProcessorTest, AliasedChannelsMatchStereo) {
  CompressorParams params;
  processor_.SetParams(params);

  const size_t n = 4096;
  std::vector<float> mono(n), left(n), right(n);
  for (size_t i = 0; i < n; ++i) {
    mono[i] = 0.7f * static_cast<float>(std::sin(2.0 * kPi * 440.0 * i / kSampleRate));
    left[i] = right[i] = mono[i];
  }
  processor_.ProcessStereo(mono.data(), mono.data(), n);

  CompressorProcessor other;
  other.Initialize(kSampleRate);
  other.SetParams(params);
  other.ProcessStereo(left.data(), right.data(), n);

  for (size_t i = 0; i < n; ++i) {
    EXPECT_FLOAT_EQ(mono[i], left[i]);
  }
}

TEST_F(CompressorProcessorTest, AutoMakeupIsDeterministicAndDoesNotDrift) {
  CompressorParams params;
  params.threshold_db = -20.0f;
  params.ratio = 4.0f;
  params.auto_makeup = true;
  processor_.SetParams(params);
  // Half the reduction a full-scale signal would get: 20 * 0.75 / 2
  EXPECT_NEAR(processor_.MakeupGainDb(), 7.5f, 1e-4f);

  // A quiet signal is simply raised by the makeup, at once and for good
  const size_t n = static_cast<size_t>(kSampleRate) * 3;
  const float amp = 0.01f;  // -40 dB
  std::vector<float> left(n, amp), right(n, amp);
  processor_.ProcessStereo(left.data(), right.data(), n);
  const float expected = amp * std::pow(10.0f, 7.5f / 20.0f);
  EXPECT_NEAR(left[10000], expected, 1e-4f);
  EXPECT_NEAR(left[n / 2], expected, 1e-4f);
  EXPECT_NEAR(left[n - 1], expected, 1e-4f);
}

TEST_F(CompressorProcessorTest, MakeupChangesAreSmoothed) {
  CompressorParams params;
  params.makeup_gain_db = 0.0f;
  processor_.SetParams(params);
  std::vector<float> left(64, 0.1f), right(64, 0.1f);
  processor_.ProcessStereo(left.data(), right.data(), 64);

  params.makeup_gain_db = 12.0f;
  processor_.SetParams(params);
  std::vector<float> l2(8192, 0.1f), r2(8192, 0.1f);
  processor_.ProcessStereo(l2.data(), r2.data(), 8192);

  // No step: first sample after the change is still close to the old gain
  EXPECT_LT(l2[0], 0.12f);
  // Monotonic ramp to the new gain, settled well within 200 ms
  for (size_t i = 1; i < 8192; ++i) EXPECT_GE(l2[i], l2[i - 1] - 1e-6f);
  EXPECT_NEAR(l2[8191], 0.1f * std::pow(10.0f, 12.0f / 20.0f), 1e-3f);
}

TEST_F(CompressorProcessorTest, MeterReportsDeepestReductionInBlock) {
  CompressorParams params;
  params.threshold_db = -20.0f;
  params.ratio = 4.0f;
  params.attack_ms = 0.1f;
  params.release_ms = 10.0f;
  processor_.SetParams(params);

  // Loud for the first half of the block, silent for the second half. The
  // hold plus release leave the last sample partially recovered, but the meter
  // must still show the deepest value reached.
  std::vector<float> left(4096, 0.0f), right(4096, 0.0f);
  for (size_t i = 0; i < 2048; ++i) left[i] = right[i] = 0.5f;
  processor_.ProcessStereo(left.data(), right.data(), left.size());
  EXPECT_LT(processor_.GetGainReduction(), -10.0f);
}

TEST_F(CompressorProcessorTest, ResetClearsState) {
  CompressorParams params;
  params.threshold_db = -20.0f;
  params.ratio = 4.0f;
  params.attack_ms = 50.0f;
  params.release_ms = 200.0f;
  processor_.SetParams(params);

  // Process signal to build up envelope state
  std::vector<float> left(512, 0.5f);
  std::vector<float> right(512, 0.5f);
  processor_.ProcessStereo(left.data(), right.data(), 512);

  // Reset should clear envelope
  processor_.Reset();

  // Process again - should behave exactly like the first time
  std::vector<float> left2(512, 0.5f);
  std::vector<float> right2(512, 0.5f);
  processor_.ProcessStereo(left2.data(), right2.data(), 512);

  CompressorProcessor fresh;
  fresh.Initialize(kSampleRate);
  fresh.SetParams(params);
  std::vector<float> left3(512, 0.5f);
  std::vector<float> right3(512, 0.5f);
  fresh.ProcessStereo(left3.data(), right3.data(), 512);

  for (size_t i = 0; i < 512; ++i) {
    EXPECT_FLOAT_EQ(left2[i], left3[i]);
  }
}

TEST_F(CompressorProcessorTest, StereoLinkingWorks) {
  CompressorParams params;
  params.threshold_db = -20.0f;
  params.ratio = 4.0f;
  params.attack_ms = 1.0f;
  params.release_ms = 100.0f;
  processor_.SetParams(params);

  // Left channel loud, right channel quiet
  std::vector<float> left(512, 0.5f);
  std::vector<float> right(512, 0.1f);

  processor_.ProcessStereo(left.data(), right.data(), 512);

  // Both channels should be compressed based on the louder channel
  // This is stereo-linked compression
  for (size_t i = 100; i < 512; ++i) {
    float left_gain = left[i] / 0.5f;
    float right_gain = right[i] / 0.1f;
    // Gains should be similar due to stereo linking
    EXPECT_NEAR(left_gain, right_gain, 0.1f);
  }
}

TEST_F(CompressorProcessorTest, AttackTimeAffectsResponse) {
  CompressorParams fast_attack;
  fast_attack.threshold_db = -20.0f;
  fast_attack.ratio = 8.0f;
  fast_attack.attack_ms = 0.5f;
  fast_attack.release_ms = 100.0f;

  CompressorParams slow_attack;
  slow_attack.threshold_db = -20.0f;
  slow_attack.ratio = 8.0f;
  slow_attack.attack_ms = 50.0f;
  slow_attack.release_ms = 100.0f;

  std::vector<float> signal(512, 0.5f);

  CompressorProcessor fast_proc;
  fast_proc.Initialize(kSampleRate);
  fast_proc.SetParams(fast_attack);
  std::vector<float> left_fast = signal;
  std::vector<float> right_fast = signal;
  fast_proc.ProcessStereo(left_fast.data(), right_fast.data(), 512);

  CompressorProcessor slow_proc;
  slow_proc.Initialize(kSampleRate);
  slow_proc.SetParams(slow_attack);
  std::vector<float> left_slow = signal;
  std::vector<float> right_slow = signal;
  slow_proc.ProcessStereo(left_slow.data(), right_slow.data(), 512);

  // Fast attack should compress more quickly
  // Check early samples
  float fast_reduction = 0.5f - std::abs(left_fast[50]);
  float slow_reduction = 0.5f - std::abs(left_slow[50]);
  EXPECT_GT(fast_reduction, slow_reduction * 0.5f);
}

TEST_F(CompressorProcessorTest, ExtremeRatioWorks) {
  CompressorParams params;
  params.threshold_db = -20.0f;
  params.ratio = 20.0f;  // Extreme ratio (near limiting)
  params.attack_ms = 1.0f;
  params.release_ms = 100.0f;
  processor_.SetParams(params);

  std::vector<float> left(512, 0.5f);
  std::vector<float> right(512, 0.5f);

  processor_.ProcessStereo(left.data(), right.data(), 512);

  // Should heavily compress/limit
  for (size_t i = 100; i < 512; ++i) {
    EXPECT_LT(std::abs(left[i]), 0.2f);
    EXPECT_LT(std::abs(right[i]), 0.2f);
  }
}

}  // namespace
}  // namespace fast_compressor
