// Copyright 2025
// Accuracy tests for the fast log2 / exp2 used by the compressor's gain
// computer, against the standard library in double precision.

#include "fast_math.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

namespace fast_compressor {
namespace fast_math {
namespace {

TEST(FastMathTest, Log2MatchesStdAcrossDetectorRange) {
  // Everything the detector can see: the -96 dB floor up to far above 0 dBFS
  double max_err = 0.0;
  for (double x = 1e-5; x < 1e3; x *= 1.0007) {
    const float xf = static_cast<float>(x);
    const double err = std::abs(static_cast<double>(FastLog2(xf)) -
                                std::log2(static_cast<double>(xf)));
    max_err = std::max(max_err, err);
  }
  // 2e-5 in log2 units is 1.2e-4 dB
  EXPECT_LT(max_err, 2e-5);
}

TEST(FastMathTest, Log2IsExactAtPowersOfTwo) {
  for (int e = -20; e <= 20; ++e) {
    EXPECT_EQ(FastLog2(std::ldexp(1.0f, e)), static_cast<float>(e));
  }
}

TEST(FastMathTest, Exp2MatchesStdAcrossGainRange) {
  // Gain reduction of 0 to 120 dB is 0 to -20 here; test much wider
  double max_rel = 0.0;
  for (double x = -60.0; x <= 60.0; x += 0.0013) {
    const float xf = static_cast<float>(x);
    const double rel =
        std::abs(static_cast<double>(FastExp2(xf)) /
                     std::exp2(static_cast<double>(xf)) -
                 1.0);
    max_rel = std::max(max_rel, rel);
  }
  // 1e-6 relative is 9e-6 dB
  EXPECT_LT(max_rel, 1e-6);
}

TEST(FastMathTest, Exp2IsExactAtIntegersAndClamps) {
  for (int n = -126; n <= 126; ++n) {
    EXPECT_EQ(FastExp2(static_cast<float>(n)), std::exp2(static_cast<float>(n)));
  }
  EXPECT_EQ(FastExp2(-1000.0f), FastExp2(-126.0f));
  EXPECT_EQ(FastExp2(1000.0f), FastExp2(126.0f));
}

#if COMPRESSOR_FAST_MATH_AVX2
TEST(FastMathTest, Avx2LanesMatchScalar) {
  alignas(32) const float log_in[8] = {1.5849e-5f, 0.001f, 0.1f, 0.5f,
                                       1.0f,       1.3f,   7.7f, 123.0f};
  alignas(32) float out[8];
  _mm256_store_ps(out, FastLog2(_mm256_load_ps(log_in)));
  for (int i = 0; i < 8; ++i) {
    const float ref = FastLog2(log_in[i]);
    EXPECT_NEAR(out[i], ref, 1e-6f * (1.0f + std::abs(ref))) << "lane " << i;
  }

  alignas(32) const float exp_in[8] = {-125.5f, -20.0f, -3.3f, -0.5f,
                                       0.0f,    0.25f,  7.9f,  60.0f};
  _mm256_store_ps(out, FastExp2(_mm256_load_ps(exp_in)));
  for (int i = 0; i < 8; ++i) {
    const float ref = FastExp2(exp_in[i]);
    EXPECT_NEAR(out[i], ref, 1e-6f * std::abs(ref)) << "lane " << i;
  }
}
#endif

}  // namespace
}  // namespace fast_math
}  // namespace fast_compressor
