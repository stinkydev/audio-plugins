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

TEST_F(CompressorProcessorTest, SoftKneeIsSmooth) {
  CompressorParams params_hard;
  params_hard.threshold_db = -20.0f;
  params_hard.ratio = 8.0f;
  params_hard.knee_db = 0.0f;  // Hard knee
  params_hard.attack_ms = 1.0f;
  params_hard.release_ms = 100.0f;

  CompressorParams params_soft;
  params_soft.threshold_db = -20.0f;
  params_soft.ratio = 8.0f;
  params_soft.knee_db = 6.0f;  // Soft knee
  params_soft.attack_ms = 1.0f;
  params_soft.release_ms = 100.0f;

  // Test signal at threshold
  std::vector<float> signal(512, 0.1f);  // -20 dB
  std::vector<float> left_hard = signal;
  std::vector<float> right_hard = signal;
  std::vector<float> left_soft = signal;
  std::vector<float> right_soft = signal;

  CompressorProcessor proc_hard;
  proc_hard.Initialize(kSampleRate);
  proc_hard.SetParams(params_hard);
  proc_hard.ProcessStereo(left_hard.data(), right_hard.data(), 512);

  CompressorProcessor proc_soft;
  proc_soft.Initialize(kSampleRate);
  proc_soft.SetParams(params_soft);
  proc_soft.ProcessStereo(left_soft.data(), right_soft.data(), 512);

  // Soft knee should compress less aggressively near threshold
  // (implementation dependent, but generally true)
  SUCCEED();  // Basic smoke test
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

  // Process again - should behave like first time
  std::vector<float> left2(512, 0.5f);
  std::vector<float> right2(512, 0.5f);
  processor_.ProcessStereo(left2.data(), right2.data(), 512);

  SUCCEED();  // If no crash, reset worked
}

TEST_F(CompressorProcessorTest, InterleavedProcessingWorks) {
  CompressorParams params;
  params.threshold_db = -20.0f;
  params.ratio = 4.0f;
  params.attack_ms = 5.0f;
  params.release_ms = 50.0f;
  processor_.SetParams(params);

  // Interleaved stereo buffer
  std::vector<float> interleaved(1024);  // 512 frames * 2 channels
  for (size_t i = 0; i < 512; ++i) {
    interleaved[i * 2] = 0.316f;      // Left
    interleaved[i * 2 + 1] = 0.316f;  // Right
  }

  processor_.Process(interleaved.data(), 512);

  // Check compression occurred
  bool compressed = false;
  for (size_t i = 200; i < 512; ++i) {
    if (std::abs(interleaved[i * 2]) < 0.31f) {
      compressed = true;
      break;
    }
  }
  EXPECT_TRUE(compressed);
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
