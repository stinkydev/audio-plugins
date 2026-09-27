// Copyright 2025
// Unit tests for DelayProcessor

#include "delay_processor.h"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

namespace stinky_delay {
namespace {

constexpr double kSampleRate = 48000.0;

int FirstIndexAbove(const std::vector<float>& v, float level) {
  for (size_t i = 0; i < v.size(); ++i) {
    if (std::abs(v[i]) > level) return static_cast<int>(i);
  }
  return -1;
}

class DelayProcessorTest : public ::testing::Test {
 protected:
  void SetUp() override { processor_.Initialize(kSampleRate); }

  void Set(float delay_ms, float mix) {
    DelayParams params;
    params.delay_time_ms = delay_ms;
    params.mix = mix;
    processor_.SetParams(params);
  }

  DelayProcessor processor_;
};

TEST_F(DelayProcessorTest, ZeroDelayIsTrulyZero) {
  Set(0.0f, 1.0f);
  EXPECT_EQ(processor_.GetDelaySamples(), 0u);

  std::vector<float> left(256), right(256);
  for (size_t i = 0; i < left.size(); ++i) {
    left[i] = static_cast<float>(i) * 0.001f;
    right[i] = -left[i];
  }
  const std::vector<float> in_left = left;
  const std::vector<float> in_right = right;
  processor_.ProcessStereo(left.data(), right.data(), 256);
  for (size_t i = 0; i < left.size(); ++i) {
    EXPECT_FLOAT_EQ(left[i], in_left[i]);
    EXPECT_FLOAT_EQ(right[i], in_right[i]);
  }
}

TEST_F(DelayProcessorTest, ZeroDelayHalfMixDoesNotCombFilter) {
  Set(0.0f, 0.5f);
  // A 20 kHz tone through a one-sample delay at 50% mix would lose 12 dB.
  const size_t n = 4800;
  std::vector<float> left(n), right(n);
  for (size_t i = 0; i < n; ++i) {
    left[i] = right[i] = static_cast<float>(std::sin(2.0 * 3.14159265358979 * 20000.0 * i / kSampleRate));
  }
  const std::vector<float> in = left;
  processor_.ProcessStereo(left.data(), right.data(), n);
  for (size_t i = 0; i < n; ++i) EXPECT_NEAR(left[i], in[i], 1e-6f);
}

TEST_F(DelayProcessorTest, DelayIsExactInSamples) {
  Set(100.0f, 1.0f);
  EXPECT_EQ(processor_.GetDelaySamples(), 4800u);

  std::vector<float> left(9600, 0.0f), right(9600, 0.0f);
  left[0] = right[0] = 1.0f;
  processor_.ProcessStereo(left.data(), right.data(), 9600);
  EXPECT_EQ(FirstIndexAbove(left, 0.5f), 4800);
  EXPECT_EQ(FirstIndexAbove(right, 0.5f), 4800);
  EXPECT_FLOAT_EQ(left[4800], 1.0f);
}

TEST_F(DelayProcessorTest, DelayTimeRoundsToNearestSample) {
  Set(33.3333f, 1.0f);  // 1599.998 samples
  EXPECT_EQ(processor_.GetDelaySamples(), 1600u);
  Set(0.01f, 1.0f);     // 0.48 samples
  EXPECT_EQ(processor_.GetDelaySamples(), 0u);
  Set(0.02f, 1.0f);     // 0.96 samples
  EXPECT_EQ(processor_.GetDelaySamples(), 1u);
}

TEST_F(DelayProcessorTest, MaximumDelayFitsInBuffer) {
  Set(2000.0f, 1.0f);
  EXPECT_EQ(processor_.GetDelaySamples(), 96000u);
  const size_t n = 96000 + 480;
  std::vector<float> left(n, 0.0f), right(n, 0.0f);
  left[0] = right[0] = 1.0f;
  processor_.ProcessStereo(left.data(), right.data(), static_cast<uint32_t>(n));
  EXPECT_EQ(FirstIndexAbove(left, 0.5f), 96000);
}

TEST_F(DelayProcessorTest, MixBlendsDryAndWet) {
  Set(10.0f, 0.25f);  // 480 samples
  std::vector<float> left(2000, 0.0f), right(2000, 0.0f);
  left[100] = right[100] = 1.0f;
  processor_.ProcessStereo(left.data(), right.data(), 2000);
  EXPECT_FLOAT_EQ(left[100], 0.75f);
  EXPECT_FLOAT_EQ(left[580], 0.25f);
  for (size_t i = 0; i < 2000; ++i) {
    if (i != 100 && i != 580) EXPECT_FLOAT_EQ(left[i], 0.0f);
  }
}

TEST_F(DelayProcessorTest, FullyDryPassesThroughUntouched) {
  Set(500.0f, 0.0f);
  std::vector<float> left(1024), right(1024);
  for (size_t i = 0; i < 1024; ++i) left[i] = right[i] = 0.3f * static_cast<float>(std::sin(0.05 * i));
  const std::vector<float> in = left;
  processor_.ProcessStereo(left.data(), right.data(), 1024);
  for (size_t i = 0; i < 1024; ++i) EXPECT_FLOAT_EQ(left[i], in[i]);
}

TEST_F(DelayProcessorTest, AliasedChannelsMatchStereo) {
  Set(20.0f, 0.5f);
  const size_t n = 4096;
  std::vector<float> mono(n), left(n), right(n);
  for (size_t i = 0; i < n; ++i) mono[i] = left[i] = right[i] = static_cast<float>(std::sin(0.02 * i));
  processor_.ProcessStereo(mono.data(), mono.data(), n);

  DelayProcessor other;
  other.Initialize(kSampleRate);
  other.SetParams(processor_.GetParams());
  other.ProcessStereo(left.data(), right.data(), n);
  for (size_t i = 0; i < n; ++i) EXPECT_FLOAT_EQ(mono[i], left[i]);
}

TEST_F(DelayProcessorTest, ResetAndReinitializeClearHistory) {
  Set(10.0f, 1.0f);
  std::vector<float> left(4800, 1.0f), right(4800, 1.0f);
  processor_.ProcessStereo(left.data(), right.data(), 4800);

  processor_.Reset();
  std::vector<float> zl(4800, 0.0f), zr(4800, 0.0f);
  processor_.ProcessStereo(zl.data(), zr.data(), 4800);
  for (float v : zl) EXPECT_FLOAT_EQ(v, 0.0f);

  processor_.ProcessStereo(left.data(), right.data(), 4800);
  processor_.Initialize(kSampleRate);
  std::fill(zl.begin(), zl.end(), 0.0f);
  processor_.ProcessStereo(zl.data(), zr.data(), 4800);
  for (float v : zl) EXPECT_FLOAT_EQ(v, 0.0f);
}

}  // namespace
}  // namespace stinky_delay
