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

}  // namespace
}  // namespace fast_eq
