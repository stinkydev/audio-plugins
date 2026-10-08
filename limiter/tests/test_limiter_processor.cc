// Copyright 2025
// DSP tests for the limiter processor
//
// These tests pin down the properties the plugin is relied on for in a live
// chain: a ceiling that is never exceeded, correct and reported latency, true
// peak detection, and sane release behaviour.

#include "limiter_processor.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace fast_limiter {
namespace {

constexpr double kPi = 3.14159265358979323846;

float DbToLin(float db) { return std::pow(10.0f, db / 20.0f); }

float MaxAbs(const std::vector<float>& v, size_t from = 0) {
  float m = 0.0f;
  for (size_t i = from; i < v.size(); ++i) m = std::max(m, std::abs(v[i]));
  return m;
}

// Process in host-sized blocks so block boundaries are exercised.
void ProcessInBlocks(LimiterProcessor& p, std::vector<float>& l,
                     std::vector<float>& r, size_t block = 64) {
  for (size_t i = 0; i < l.size(); i += block) {
    const size_t n = std::min(block, l.size() - i);
    p.ProcessStereo(l.data() + i, r.data() + i, n);
  }
}

class LimiterProcessorTest : public ::testing::TestWithParam<double> {
 protected:
  void SetUp() override {
    sample_rate_ = GetParam();
    processor_.Initialize(sample_rate_);
    LimiterParams params;
    params.threshold_db = -1.0f;
    params.output_level_db = -1.0f;
    processor_.SetParams(params);
    ceiling_ = DbToLin(-1.0f);
  }

  double sample_rate_;
  LimiterProcessor processor_;
  float ceiling_;
};

TEST_P(LimiterProcessorTest, ReportedLatencyMatchesMeasuredDelay) {
  std::vector<float> l(4096, 0.0f), r(4096, 0.0f);
  l[0] = 0.5f;
  r[0] = 0.5f;
  ProcessInBlocks(processor_, l, r);

  size_t first = l.size();
  for (size_t i = 0; i < l.size(); ++i) {
    if (std::abs(l[i]) > 0.25f) {
      first = i;
      break;
    }
  }
  EXPECT_EQ(first, processor_.GetLatencySamples());
  EXPECT_NEAR(l[first], 0.5f, 1e-5f);

  // 5 ms lookahead plus the true-peak filter delay
  const size_t expected_lookahead =
      static_cast<size_t>(std::lround(0.005 * sample_rate_));
  EXPECT_EQ(processor_.GetLookaheadSamples(), expected_lookahead);
  EXPECT_GE(processor_.GetLatencySamples(), expected_lookahead - 1);
  EXPECT_LE(processor_.GetLatencySamples(), expected_lookahead + 8);
}

TEST_P(LimiterProcessorTest, LowLevelSignalPassesThroughDelayed) {
  const size_t n = 8192;
  std::vector<float> l(n), r(n);
  for (size_t i = 0; i < n; ++i) {
    l[i] = 0.1f * static_cast<float>(std::sin(2.0 * kPi * 440.0 * i / sample_rate_));
    r[i] = -l[i];
  }
  const std::vector<float> in_l = l;
  const std::vector<float> in_r = r;
  ProcessInBlocks(processor_, l, r);

  const size_t lat = processor_.GetLatencySamples();
  for (size_t i = lat; i < n; ++i) {
    EXPECT_NEAR(l[i], in_l[i - lat], 1e-5f);
    EXPECT_NEAR(r[i], in_r[i - lat], 1e-5f);
  }
  EXPECT_NEAR(processor_.GetGainReduction(), 0.0f, 1e-3f);
}

TEST_P(LimiterProcessorTest, StepNeverExceedsCeiling) {
  std::vector<float> l(8192, 0.0f), r(8192, 0.0f);
  for (size_t i = 500; i < l.size(); ++i) {
    l[i] = 2.0f;  // +6 dB over full scale
    r[i] = 2.0f;
  }
  ProcessInBlocks(processor_, l, r);
  EXPECT_LE(MaxAbs(l), ceiling_ + 1e-6f);
  EXPECT_LE(MaxAbs(r), ceiling_ + 1e-6f);
  EXPECT_LT(processor_.GetGainReduction(), -6.0f);
}

TEST_P(LimiterProcessorTest, SingleSampleTransientNeverExceedsCeiling) {
  std::vector<float> l(8192, 0.0f), r(8192, 0.0f);
  l[1000] = 8.0f;   // +18 dB
  r[1000] = -8.0f;
  l[3000] = 0.95f;  // just under full scale, still above the -1 dB ceiling
  ProcessInBlocks(processor_, l, r);
  EXPECT_LE(MaxAbs(l), ceiling_ + 1e-6f);
  EXPECT_LE(MaxAbs(r), ceiling_ + 1e-6f);

  // The transient itself must still come out, at the ceiling
  const size_t lat = processor_.GetLatencySamples();
  EXPECT_NEAR(std::abs(l[1000 + lat]), ceiling_, 1e-3f);
}

TEST_P(LimiterProcessorTest, HotSineNeverExceedsCeiling) {
  const size_t n = static_cast<size_t>(sample_rate_);  // one second
  std::vector<float> l(n), r(n);
  for (size_t i = 0; i < n; ++i) {
    l[i] = 2.0f * static_cast<float>(std::sin(2.0 * kPi * 1000.0 * i / sample_rate_));
    r[i] = 1.5f * static_cast<float>(std::sin(2.0 * kPi * 61.0 * i / sample_rate_));
  }
  ProcessInBlocks(processor_, l, r, 37);  // odd block size on purpose
  EXPECT_LE(MaxAbs(l), ceiling_ + 1e-6f);
  EXPECT_LE(MaxAbs(r), ceiling_ + 1e-6f);
  // Should still be delivering close to the ceiling, not over-limiting
  EXPECT_GT(MaxAbs(l, n / 2), ceiling_ * 0.9f);
}

TEST_P(LimiterProcessorTest, DetectsInterSamplePeaks) {
  if (processor_.GetOversamplingFactor() == 1) {
    GTEST_SKIP() << "No oversampling at this sample rate";
  }
  // A sine at fs/4 with a 45 degree phase offset has sample values of
  // +/- 0.707 but a true peak of 1.0. Sample-peak detection would leave this
  // untouched; true-peak detection must pull it down.
  const size_t n = static_cast<size_t>(sample_rate_);
  std::vector<float> l(n), r(n);
  for (size_t i = 0; i < n; ++i) {
    l[i] = static_cast<float>(std::sin(kPi / 2.0 * i + kPi / 4.0));
    r[i] = l[i];
  }
  ProcessInBlocks(processor_, l, r);

  const float sample_peak_out = MaxAbs(l, n / 2);
  const float true_peak_out = sample_peak_out * std::sqrt(2.0f);
  // Sample-peak limiter would output 0.707; we expect ~0.63 (0.891 / sqrt 2)
  EXPECT_LT(sample_peak_out, 0.68f);
  EXPECT_NEAR(true_peak_out, ceiling_, 0.02f);
  EXPECT_LT(processor_.GetGainReduction(), -0.5f);
}

TEST_P(LimiterProcessorTest, OutputLevelBelowThresholdScalesOutput) {
  LimiterParams params;
  params.threshold_db = -1.0f;
  params.output_level_db = -7.0f;
  processor_.SetParams(params);

  const size_t n = static_cast<size_t>(sample_rate_ / 2);
  std::vector<float> l(n), r(n);
  for (size_t i = 0; i < n; ++i) {
    l[i] = r[i] = 2.0f * static_cast<float>(std::sin(2.0 * kPi * 1000.0 * i / sample_rate_));
  }
  ProcessInBlocks(processor_, l, r);
  const float out_ceiling = DbToLin(-7.0f);
  EXPECT_LE(MaxAbs(l), out_ceiling + 1e-6f);
  EXPECT_GT(MaxAbs(l, n / 2), out_ceiling * 0.9f);
}

TEST_P(LimiterProcessorTest, ReleasesAfterSustainedLimiting) {
  // Two seconds of heavy limiting (crowd roar), then a quiet signal. The gain
  // must recover to unity within a couple of seconds, and must not snap back
  // instantly (the slow stage should be engaged after sustained limiting).
  const size_t burst = static_cast<size_t>(2.0 * sample_rate_);
  const size_t tail = static_cast<size_t>(3.0 * sample_rate_);
  std::vector<float> l(burst + tail), r(burst + tail);
  for (size_t i = 0; i < l.size(); ++i) {
    const float amp = (i < burst) ? 3.0f : 0.5f;
    l[i] = r[i] = amp * static_cast<float>(std::sin(2.0 * kPi * 440.0 * i / sample_rate_));
  }
  ProcessInBlocks(processor_, l, r);

  EXPECT_LE(MaxAbs(l), ceiling_ + 1e-6f);

  // Shortly after the burst the quiet signal should still be attenuated
  const size_t soon = burst + static_cast<size_t>(0.05 * sample_rate_);
  const size_t window = static_cast<size_t>(0.01 * sample_rate_);
  float soon_peak = 0.0f;
  for (size_t i = soon; i < soon + window; ++i) soon_peak = std::max(soon_peak, std::abs(l[i]));
  EXPECT_LT(soon_peak, 0.45f);

  // ... and fully recovered well before the end
  const size_t late = burst + static_cast<size_t>(2.5 * sample_rate_);
  float late_peak = 0.0f;
  for (size_t i = late; i < late + window; ++i) late_peak = std::max(late_peak, std::abs(l[i]));
  EXPECT_NEAR(late_peak, 0.5f, 0.01f);
}

TEST_P(LimiterProcessorTest, ReleaseSettlesExactlyAtUnity) {
  // After limiting stops the gain must return to exactly 1.0, not stall a
  // fraction of a dB below it. The envelopes used to be float one-pole
  // smoothers, whose update k * (1 - env) underflows below half an ulp of env
  // at env ~ 0.999, leaving a permanent residual reduction on the meter.
  const size_t burst = static_cast<size_t>(0.5 * sample_rate_);
  std::vector<float> l(burst, 2.0f), r(burst, 2.0f);
  ProcessInBlocks(processor_, l, r);
  ASSERT_LT(processor_.GetGainReduction(), -1.0f);

  // 20 time constants of the slow (500 ms) release
  const size_t tail = static_cast<size_t>(10.0 * sample_rate_);
  std::vector<float> zl(tail, 0.0f), zr(tail, 0.0f);
  ProcessInBlocks(processor_, zl, zr);
  EXPECT_FLOAT_EQ(processor_.GetGainReduction(), 0.0f);

  // ... and a signal under the ceiling then passes at exactly unity gain
  std::vector<float> ql(4096, 0.5f), qr(4096, 0.5f);
  ProcessInBlocks(processor_, ql, qr);
  EXPECT_FLOAT_EQ(MaxAbs(ql), 0.5f);
}

TEST_P(LimiterProcessorTest, ResetClearsState) {
  std::vector<float> l(4096, 2.0f), r(4096, 2.0f);
  ProcessInBlocks(processor_, l, r);
  EXPECT_LT(processor_.GetGainReduction(), -1.0f);

  processor_.Reset();
  EXPECT_FLOAT_EQ(processor_.GetGainReduction(), 0.0f);

  std::vector<float> zl(4096, 0.0f), zr(4096, 0.0f);
  ProcessInBlocks(processor_, zl, zr);
  EXPECT_FLOAT_EQ(MaxAbs(zl), 0.0f);
  EXPECT_FLOAT_EQ(MaxAbs(zr), 0.0f);
}

TEST_P(LimiterProcessorTest, InterleavedMatchesSeparateChannels) {
  const size_t n = 4096;
  std::vector<float> l(n), r(n), inter(2 * n);
  for (size_t i = 0; i < n; ++i) {
    l[i] = 1.8f * static_cast<float>(std::sin(2.0 * kPi * 300.0 * i / sample_rate_));
    r[i] = 1.2f * static_cast<float>(std::cos(2.0 * kPi * 700.0 * i / sample_rate_));
    inter[2 * i] = l[i];
    inter[2 * i + 1] = r[i];
  }
  processor_.ProcessStereo(l.data(), r.data(), n);

  LimiterProcessor other;
  other.Initialize(sample_rate_);
  other.SetParams(processor_.GetParams());
  other.Process(inter.data(), n);

  for (size_t i = 0; i < n; ++i) {
    EXPECT_FLOAT_EQ(inter[2 * i], l[i]);
    EXPECT_FLOAT_EQ(inter[2 * i + 1], r[i]);
  }
}

TEST_P(LimiterProcessorTest, AliasedChannelsForMonoOperation) {
  const size_t n = 4096;
  std::vector<float> mono(n), l(n), r(n);
  for (size_t i = 0; i < n; ++i) {
    mono[i] = 2.0f * static_cast<float>(std::sin(2.0 * kPi * 500.0 * i / sample_rate_));
    l[i] = r[i] = mono[i];
  }
  processor_.ProcessStereo(mono.data(), mono.data(), n);

  LimiterProcessor other;
  other.Initialize(sample_rate_);
  other.SetParams(processor_.GetParams());
  other.ProcessStereo(l.data(), r.data(), n);

  for (size_t i = 0; i < n; ++i) EXPECT_FLOAT_EQ(mono[i], l[i]);
}

INSTANTIATE_TEST_SUITE_P(SampleRates, LimiterProcessorTest,
                         ::testing::Values(44100.0, 48000.0, 96000.0, 192000.0));

}  // namespace
}  // namespace fast_limiter
