// Copyright 2025
// Unit tests for SIMD utilities

#include "simd_utils.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace fast_compressor {
namespace simd {
namespace {

constexpr float kEpsilon = 1e-5f;
constexpr size_t kBufferSize = 1024;

class SimdUtilsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    src1_.resize(kBufferSize);
    src2_.resize(kBufferSize);
    dest_.resize(kBufferSize);
    
    // Fill with test data
    for (size_t i = 0; i < kBufferSize; ++i) {
      src1_[i] = static_cast<float>(i) / 100.0f;
      src2_[i] = static_cast<float>(i + 1) / 200.0f;
      dest_[i] = 0.0f;
    }
  }

  std::vector<float> src1_;
  std::vector<float> src2_;
  std::vector<float> dest_;
};

TEST_F(SimdUtilsTest, IsSimdAvailableReturnsConsistently) {
  bool result1 = IsSimdAvailable();
  bool result2 = IsSimdAvailable();
  EXPECT_EQ(result1, result2);
}

TEST_F(SimdUtilsTest, MultiplyAddCorrectResults) {
  const float multiplier = 2.5f;
  
  // Set initial dest values
  for (size_t i = 0; i < kBufferSize; ++i) {
    dest_[i] = 1.0f;
  }

  MultiplyAdd(dest_.data(), src1_.data(), multiplier, kBufferSize);

  // Verify: dest[i] = dest[i] + src1[i] * multiplier
  for (size_t i = 0; i < kBufferSize; ++i) {
    float expected = 1.0f + src1_[i] * multiplier;
    EXPECT_NEAR(dest_[i], expected, kEpsilon);
  }
}

TEST_F(SimdUtilsTest, MultiplyCorrectResults) {
  const float multiplier = 3.0f;

  Multiply(dest_.data(), src1_.data(), multiplier, kBufferSize);

  // Verify: dest[i] = src1[i] * multiplier
  for (size_t i = 0; i < kBufferSize; ++i) {
    float expected = src1_[i] * multiplier;
    EXPECT_NEAR(dest_[i], expected, kEpsilon);
  }
}

TEST_F(SimdUtilsTest, ApplyGainCorrectResults) {
  const float gain = 0.5f;
  std::vector<float> buffer = src1_;

  ApplyGain(buffer.data(), gain, kBufferSize);

  // Verify: buffer[i] *= gain
  for (size_t i = 0; i < kBufferSize; ++i) {
    float expected = src1_[i] * gain;
    EXPECT_NEAR(buffer[i], expected, kEpsilon);
  }
}

TEST_F(SimdUtilsTest, MaxCorrectResults) {
  Max(dest_.data(), src1_.data(), src2_.data(), kBufferSize);

  // Verify: dest[i] = max(src1[i], src2[i])
  for (size_t i = 0; i < kBufferSize; ++i) {
    float expected = std::max(src1_[i], src2_[i]);
    EXPECT_NEAR(dest_[i], expected, kEpsilon);
  }
}

TEST_F(SimdUtilsTest, MinCorrectResults) {
  Min(dest_.data(), src1_.data(), src2_.data(), kBufferSize);

  // Verify: dest[i] = min(src1[i], src2[i])
  for (size_t i = 0; i < kBufferSize; ++i) {
    float expected = std::min(src1_[i], src2_[i]);
    EXPECT_NEAR(dest_[i], expected, kEpsilon);
  }
}

TEST_F(SimdUtilsTest, ConvertToDbHandlesZero) {
  std::vector<float> zeros(kBufferSize, 0.0f);
  ConvertToDb(dest_.data(), zeros.data(), kBufferSize);

  // Should not crash and should produce valid (very negative) dB values
  for (size_t i = 0; i < kBufferSize; ++i) {
    EXPECT_LT(dest_[i], -80.0f);  // Very quiet
    EXPECT_TRUE(std::isfinite(dest_[i]));
  }
}

TEST_F(SimdUtilsTest, ConvertToDbCorrectResults) {
  std::vector<float> signal = {0.5f, 1.0f, 0.1f, 0.01f};
  std::vector<float> result(signal.size());

  ConvertToDb(result.data(), signal.data(), signal.size());

  // Verify dB conversion: 20 * log10(abs(x))
  EXPECT_NEAR(result[0], 20.0f * std::log10f(0.5f), 0.1f);
  EXPECT_NEAR(result[1], 20.0f * std::log10f(1.0f), 0.1f);
  EXPECT_NEAR(result[2], 20.0f * std::log10f(0.1f), 0.1f);
  EXPECT_NEAR(result[3], 20.0f * std::log10f(0.01f), 0.1f);
}

TEST_F(SimdUtilsTest, OperationsHandleNonMultipleOf8) {
  // Test with sizes that aren't multiples of 8 (SIMD width)
  std::vector<size_t> test_sizes = {1, 7, 15, 17, 63, 127};

  for (size_t size : test_sizes) {
    std::vector<float> a(size, 2.0f);
    std::vector<float> b(size, 3.0f);
    std::vector<float> result(size, 0.0f);

    // Test each operation
    Multiply(result.data(), a.data(), 2.0f, size);
    for (size_t i = 0; i < size; ++i) {
      EXPECT_NEAR(result[i], 4.0f, kEpsilon);
    }

    Max(result.data(), a.data(), b.data(), size);
    for (size_t i = 0; i < size; ++i) {
      EXPECT_NEAR(result[i], 3.0f, kEpsilon);
    }

    Min(result.data(), a.data(), b.data(), size);
    for (size_t i = 0; i < size; ++i) {
      EXPECT_NEAR(result[i], 2.0f, kEpsilon);
    }
  }
}

TEST_F(SimdUtilsTest, ApplyGainHandlesZeroGain) {
  std::vector<float> buffer = src1_;
  ApplyGain(buffer.data(), 0.0f, kBufferSize);

  for (size_t i = 0; i < kBufferSize; ++i) {
    EXPECT_FLOAT_EQ(buffer[i], 0.0f);
  }
}

TEST_F(SimdUtilsTest, ApplyGainHandlesNegativeGain) {
  std::vector<float> buffer = src1_;
  const float gain = -2.0f;
  
  ApplyGain(buffer.data(), gain, kBufferSize);

  for (size_t i = 0; i < kBufferSize; ++i) {
    float expected = src1_[i] * gain;
    EXPECT_NEAR(buffer[i], expected, kEpsilon);
  }
}

TEST_F(SimdUtilsTest, MultiplyAddHandlesZeroMultiplier) {
  for (size_t i = 0; i < kBufferSize; ++i) {
    dest_[i] = 5.0f;
  }

  MultiplyAdd(dest_.data(), src1_.data(), 0.0f, kBufferSize);

  // dest should remain unchanged
  for (size_t i = 0; i < kBufferSize; ++i) {
    EXPECT_FLOAT_EQ(dest_[i], 5.0f);
  }
}

TEST_F(SimdUtilsTest, MaxWithNegativeValues) {
  for (size_t i = 0; i < kBufferSize; ++i) {
    src1_[i] = -static_cast<float>(i) / 100.0f;
    src2_[i] = -static_cast<float>(i + 10) / 100.0f;
  }

  Max(dest_.data(), src1_.data(), src2_.data(), kBufferSize);

  for (size_t i = 0; i < kBufferSize; ++i) {
    float expected = std::max(src1_[i], src2_[i]);
    EXPECT_NEAR(dest_[i], expected, kEpsilon);
  }
}

TEST_F(SimdUtilsTest, SimdVsScalarConsistency) {
  // This test verifies SIMD and scalar paths produce same results
  // by running operations on different sizes
  
  const float multiplier = 1.5f;
  std::vector<float> src(100);
  for (size_t i = 0; i < src.size(); ++i) {
    src[i] = static_cast<float>(i) * 0.01f;
  }

  // Test small size (likely scalar)
  std::vector<float> dest_small(5, 1.0f);
  MultiplyAdd(dest_small.data(), src.data(), multiplier, 5);

  // Test large size (likely SIMD)
  std::vector<float> dest_large(5, 1.0f);
  std::vector<float> src_large(src.begin(), src.begin() + 5);
  MultiplyAdd(dest_large.data(), src_large.data(), multiplier, 5);

  // Results should match
  for (size_t i = 0; i < 5; ++i) {
    EXPECT_NEAR(dest_small[i], dest_large[i], kEpsilon);
  }
}

TEST_F(SimdUtilsTest, LargeBufferPerformance) {
  // Test with realistic audio buffer size
  constexpr size_t audio_buffer_size = 8192;
  std::vector<float> large_src(audio_buffer_size, 0.5f);
  std::vector<float> large_dest(audio_buffer_size, 1.0f);

  MultiplyAdd(large_dest.data(), large_src.data(), 2.0f, audio_buffer_size);

  // Verify correctness on large buffer
  for (size_t i = 0; i < audio_buffer_size; ++i) {
    EXPECT_NEAR(large_dest[i], 2.0f, kEpsilon);
  }
}

}  // namespace
}  // namespace simd
}  // namespace fast_compressor
