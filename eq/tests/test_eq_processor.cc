// Copyright 2025
// Unit tests for EqProcessor

#include "eq_processor.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>
#include <numbers>

namespace fast_eq {
namespace {

constexpr double kSampleRate = 44100.0;
constexpr float kEpsilon = 1e-5f;

class EqProcessorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    processor_.Initialize(kSampleRate);
  }

  EqProcessor processor_;
};

TEST_F(EqProcessorTest, InitializeSetsSampleRate) {
  processor_.Initialize(48000.0);
  SUCCEED();
}

TEST_F(EqProcessorTest, ProcessSilenceRemainsUnchanged) {
  std::vector<float> left(512, 0.0f);
  std::vector<float> right(512, 0.0f);

  processor_.ProcessStereo(left.data(), right.data(), 512);

  for (size_t i = 0; i < left.size(); ++i) {
    EXPECT_FLOAT_EQ(left[i], 0.0f);
    EXPECT_FLOAT_EQ(right[i], 0.0f);
  }
}

TEST_F(EqProcessorTest, BypassLeavesSignalUnchanged) {
  EqParams params = processor_.GetParams();
  params.bypass = true;
  processor_.SetParams(params);

  std::vector<float> left(512, 0.5f);
  std::vector<float> right(512, 0.5f);
  
  processor_.ProcessStereo(left.data(), right.data(), 512);

  for (size_t i = 0; i < left.size(); ++i) {
    EXPECT_FLOAT_EQ(left[i], 0.5f);
    EXPECT_FLOAT_EQ(right[i], 0.5f);
  }
}

TEST_F(EqProcessorTest, DisabledBandsDoNotAffectSignal) {
  EqParams params = processor_.GetParams();
  
  // Disable all bands
  for (auto& band : params.bands) {
    band.enabled = false;
  }
  processor_.SetParams(params);

  std::vector<float> left(512, 0.5f);
  std::vector<float> right(512, 0.5f);
  
  processor_.ProcessStereo(left.data(), right.data(), 512);

  for (size_t i = 0; i < left.size(); ++i) {
    EXPECT_NEAR(left[i], 0.5f, 0.001f);
    EXPECT_NEAR(right[i], 0.5f, 0.001f);
  }
}

TEST_F(EqProcessorTest, OutputGainAffectsLevel) {
  EqParams params = processor_.GetParams();
  params.output_gain_db = 6.0f;  // +6 dB = 2x amplitude
  
  // Disable all bands for clean test
  for (auto& band : params.bands) {
    band.enabled = false;
  }
  processor_.SetParams(params);

  std::vector<float> left(512, 0.5f);
  std::vector<float> right(512, 0.5f);
  
  processor_.ProcessStereo(left.data(), right.data(), 512);

  // +6 dB should roughly double the amplitude
  for (size_t i = 0; i < left.size(); ++i) {
    EXPECT_NEAR(left[i], 1.0f, 0.05f);
    EXPECT_NEAR(right[i], 1.0f, 0.05f);
  }
}

TEST_F(EqProcessorTest, PeakFilterBoostsAtFrequency) {
  EqParams params = processor_.GetParams();
  
  // Configure single peak band with boost
  params.bands[0].type = FilterType::kBell;
  params.bands[0].frequency_hz = 1000.0f;
  params.bands[0].gain_db = 12.0f;
  params.bands[0].q = 1.0f;
  params.bands[0].enabled = true;
  
  // Disable other bands
  for (size_t i = 1; i < 4; ++i) {
    params.bands[i].enabled = false;
  }
  
  processor_.SetParams(params);

  // Generate 1 kHz sine wave
  std::vector<float> left(512);
  std::vector<float> right(512);
  const float freq = 1000.0f;
  const float omega = 2.0f * std::numbers::pi_v<float> * freq / static_cast<float>(kSampleRate);
  
  for (size_t i = 0; i < left.size(); ++i) {
    left[i] = right[i] = 0.5f * std::sin(omega * i);
  }
  
  // Calculate RMS before
  float rms_before = 0.0f;
  for (size_t i = 100; i < left.size(); ++i) {
    rms_before += left[i] * left[i];
  }
  rms_before = std::sqrt(rms_before / (left.size() - 100));
  
  processor_.ProcessStereo(left.data(), right.data(), 512);
  
  // Calculate RMS after (skip initial samples for filter warmup)
  float rms_after = 0.0f;
  for (size_t i = 100; i < left.size(); ++i) {
    rms_after += left[i] * left[i];
  }
  rms_after = std::sqrt(rms_after / (left.size() - 100));
  
  // With +12 dB boost, signal should be significantly louder
  EXPECT_GT(rms_after, rms_before * 2.0f);
}

TEST_F(EqProcessorTest, LowPassFiltersHighFrequencies) {
  EqParams params = processor_.GetParams();
  
  // Configure high cut (low pass) filter at 1 kHz
  params.bands[0].type = FilterType::kHighCut;
  params.bands[0].frequency_hz = 1000.0f;
  params.bands[0].q = 0.707f;
  params.bands[0].enabled = true;
  
  // Disable other bands
  for (size_t i = 1; i < 4; ++i) {
    params.bands[i].enabled = false;
  }
  
  processor_.SetParams(params);

  // Generate 5 kHz sine wave (above cutoff)
  std::vector<float> left(512);
  std::vector<float> right(512);
  const float freq = 5000.0f;
  const float omega = 2.0f * std::numbers::pi_v<float> * freq / static_cast<float>(kSampleRate);
  
  for (size_t i = 0; i < left.size(); ++i) {
    left[i] = right[i] = 0.5f * std::sin(omega * i);
  }
  
  float rms_before = 0.0f;
  for (const auto& sample : left) {
    rms_before += sample * sample;
  }
  rms_before = std::sqrt(rms_before / left.size());
  
  processor_.ProcessStereo(left.data(), right.data(), 512);
  
  // Calculate RMS after (skip initial samples)
  float rms_after = 0.0f;
  for (size_t i = 100; i < left.size(); ++i) {
    rms_after += left[i] * left[i];
  }
  rms_after = std::sqrt(rms_after / (left.size() - 100));
  
  // High frequency should be attenuated significantly
  EXPECT_LT(rms_after, rms_before * 0.5f);
}

TEST_F(EqProcessorTest, HighPassFiltersLowFrequencies) {
  EqParams params = processor_.GetParams();
  
  // Configure low cut (high pass) filter at 1 kHz
  params.bands[0].type = FilterType::kLowCut;
  params.bands[0].frequency_hz = 1000.0f;
  params.bands[0].q = 0.707f;
  params.bands[0].enabled = true;
  
  // Disable other bands
  for (size_t i = 1; i < 4; ++i) {
    params.bands[i].enabled = false;
  }
  
  processor_.SetParams(params);

  // Generate 200 Hz sine wave (below cutoff)
  std::vector<float> left(512);
  std::vector<float> right(512);
  const float freq = 200.0f;
  const float omega = 2.0f * std::numbers::pi_v<float> * freq / static_cast<float>(kSampleRate);
  
  for (size_t i = 0; i < left.size(); ++i) {
    left[i] = right[i] = 0.5f * std::sin(omega * i);
  }
  
  float rms_before = 0.0f;
  for (const auto& sample : left) {
    rms_before += sample * sample;
  }
  rms_before = std::sqrt(rms_before / left.size());
  
  processor_.ProcessStereo(left.data(), right.data(), 512);
  
  // Calculate RMS after (skip initial samples)
  float rms_after = 0.0f;
  for (size_t i = 100; i < left.size(); ++i) {
    rms_after += left[i] * left[i];
  }
  rms_after = std::sqrt(rms_after / (left.size() - 100));
  
  // Low frequency should be attenuated significantly
  EXPECT_LT(rms_after, rms_before * 0.5f);
}

TEST_F(EqProcessorTest, ResetClearsFilterState) {
  EqParams params = processor_.GetParams();
  params.bands[0].type = FilterType::kBell;
  params.bands[0].frequency_hz = 1000.0f;
  params.bands[0].gain_db = 6.0f;
  params.bands[0].enabled = true;
  processor_.SetParams(params);

  // Process some samples
  std::vector<float> left(256, 0.5f);
  std::vector<float> right(256, 0.5f);
  processor_.ProcessStereo(left.data(), right.data(), 256);

  // Reset
  processor_.Reset();

  // Process silence - should remain silent after reset
  std::fill(left.begin(), left.end(), 0.0f);
  std::fill(right.begin(), right.end(), 0.0f);
  processor_.ProcessStereo(left.data(), right.data(), 256);

  for (size_t i = 0; i < left.size(); ++i) {
    EXPECT_NEAR(left[i], 0.0f, kEpsilon);
    EXPECT_NEAR(right[i], 0.0f, kEpsilon);
  }
}

// Steady-state magnitude of a sine through the processor, in dB.
float GainAtDb(EqProcessor& processor, double frequency, double sample_rate) {
  const size_t n = static_cast<size_t>(sample_rate);
  std::vector<float> left(n), right(n);
  for (size_t i = 0; i < n; ++i) {
    left[i] = right[i] = static_cast<float>(
        std::sin(2.0 * std::numbers::pi * frequency * i / sample_rate));
  }
  processor.Reset();
  processor.ProcessStereo(left.data(), right.data(), n);
  double re = 0.0, im = 0.0;
  const size_t start = n / 2;
  for (size_t i = start; i < n; ++i) {
    const double phase = 2.0 * std::numbers::pi * frequency * i / sample_rate;
    re += left[i] * std::cos(phase);
    im += left[i] * std::sin(phase);
  }
  const double magnitude = 2.0 * std::sqrt(re * re + im * im) / (n - start);
  return static_cast<float>(20.0 * std::log10(magnitude));
}

EqParams SingleBand(size_t band, FilterType type, float frequency, float gain,
                    float q) {
  EqParams params;
  for (auto& b : params.bands) b.enabled = false;
  params.bands[band].enabled = true;
  params.bands[band].type = type;
  params.bands[band].frequency_hz = frequency;
  params.bands[band].gain_db = gain;
  params.bands[band].q = q;
  return params;
}

TEST_F(EqProcessorTest, FilterResponsesMatchTextbook) {
  processor_.Initialize(48000.0);

  processor_.SetParams(SingleBand(1, FilterType::kBell, 1000.0f, 6.0f, 1.0f));
  EXPECT_NEAR(GainAtDb(processor_, 1000.0, 48000.0), 6.0f, 0.05f);
  EXPECT_NEAR(GainAtDb(processor_, 100.0, 48000.0), 0.0f, 0.15f);

  processor_.SetParams(SingleBand(0, FilterType::kLowShelf, 100.0f, 6.0f, 0.707f));
  EXPECT_NEAR(GainAtDb(processor_, 20.0, 48000.0), 6.0f, 0.1f);
  EXPECT_NEAR(GainAtDb(processor_, 100.0, 48000.0), 3.0f, 0.1f);
  EXPECT_NEAR(GainAtDb(processor_, 10000.0, 48000.0), 0.0f, 0.05f);

  processor_.SetParams(SingleBand(3, FilterType::kHighShelf, 8000.0f, -6.0f, 0.707f));
  EXPECT_NEAR(GainAtDb(processor_, 100.0, 48000.0), 0.0f, 0.05f);
  EXPECT_NEAR(GainAtDb(processor_, 8000.0, 48000.0), -3.0f, 0.1f);

  processor_.SetParams(SingleBand(0, FilterType::kLowCut, 100.0f, 0.0f, 0.707f));
  EXPECT_NEAR(GainAtDb(processor_, 100.0, 48000.0), -3.0f, 0.1f);
  EXPECT_NEAR(GainAtDb(processor_, 50.0, 48000.0), -12.3f, 0.3f);  // 12 dB/oct

  processor_.SetParams(SingleBand(3, FilterType::kHighCut, 8000.0f, 0.0f, 0.707f));
  EXPECT_NEAR(GainAtDb(processor_, 8000.0, 48000.0), -3.0f, 0.1f);
  EXPECT_NEAR(GainAtDb(processor_, 1000.0, 48000.0), 0.0f, 0.05f);
}

TEST_F(EqProcessorTest, StableAtParameterExtremes) {
  // 20 kHz at 44.1 kHz with maximum Q and gain, plus a frequency above
  // Nyquist at a low sample rate, must both stay finite and decay.
  for (double sample_rate : {44100.0, 22050.0}) {
    processor_.Initialize(sample_rate);
    processor_.SetParams(SingleBand(1, FilterType::kBell, 20000.0f, 24.0f, 10.0f));
    std::vector<float> left(8192, 0.0f), right(8192, 0.0f);
    left[0] = right[0] = 1.0f;
    processor_.ProcessStereo(left.data(), right.data(), left.size());
    for (float v : left) ASSERT_TRUE(std::isfinite(v));
    float tail = 0.0f;
    for (size_t i = 6000; i < 8192; ++i) tail = std::max(tail, std::abs(left[i]));
    EXPECT_LT(tail, 1e-3f);
  }
}

TEST_F(EqProcessorTest, UnchangedParamsDoNotDisturbFilterState) {
  EqParams params = SingleBand(1, FilterType::kBell, 1000.0f, 12.0f, 2.0f);
  processor_.SetParams(params);

  // Run a sine, then keep re-applying identical params every block. The
  // output must be identical to a run without the re-applications.
  const size_t n = 4096;
  std::vector<float> a_l(n), a_r(n), b_l(n), b_r(n);
  for (size_t i = 0; i < n; ++i) {
    a_l[i] = a_r[i] = b_l[i] = b_r[i] =
        static_cast<float>(std::sin(2.0 * std::numbers::pi * 1000.0 * i / kSampleRate));
  }
  for (size_t i = 0; i < n; i += 64) {
    processor_.SetParams(params);
    processor_.ProcessStereo(a_l.data() + i, a_r.data() + i, 64);
  }
  EqProcessor other;
  other.Initialize(kSampleRate);
  other.SetParams(params);
  other.ProcessStereo(b_l.data(), b_r.data(), n);
  for (size_t i = 0; i < n; ++i) EXPECT_FLOAT_EQ(a_l[i], b_l[i]);
}

TEST_F(EqProcessorTest, AliasedChannelsMatchStereo) {
  EqParams params = SingleBand(2, FilterType::kBell, 2000.0f, -6.0f, 1.5f);
  processor_.SetParams(params);
  const size_t n = 2048;
  std::vector<float> mono(n), left(n), right(n);
  for (size_t i = 0; i < n; ++i) mono[i] = left[i] = right[i] = static_cast<float>(std::sin(0.3 * i));
  processor_.ProcessStereo(mono.data(), mono.data(), n);

  EqProcessor other;
  other.Initialize(kSampleRate);
  other.SetParams(params);
  other.ProcessStereo(left.data(), right.data(), n);
  for (size_t i = 0; i < n; ++i) EXPECT_FLOAT_EQ(mono[i], left[i]);
}

}  // namespace
}  // namespace fast_eq
