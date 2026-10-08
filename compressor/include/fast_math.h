// Copyright 2025
// Stinky Compressor - fast log2 / exp2 for the gain computer
//
// The compressor converts the detector level to dB and the smoothed gain
// reduction back to linear for every sample. Standard log10 and pow dominate
// the per-sample cost, so these approximations replace them: a few
// multiply-adds each, accurate to about 1e-7 relative, which is around 1e-6 dB
// and far below anything audible. Both have an AVX2 version (8 lanes) and a
// scalar version that performs the same operations in the same order, so
// builds with and without ENABLE_SIMD differ only by fused-multiply-add
// rounding.
//
// Preconditions: FastLog2 takes a positive normal float (the detector clamps
// its input to a -96 dB floor); FastExp2 clamps its argument to [-126, 126].

#ifndef COMPRESSOR_FAST_MATH_H_
#define COMPRESSOR_FAST_MATH_H_

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>

#if defined(USE_SIMD) && defined(__AVX2__) && \
    (defined(__FMA__) || defined(_MSC_VER))
#include <immintrin.h>
#define COMPRESSOR_FAST_MATH_AVX2 1
#else
#define COMPRESSOR_FAST_MATH_AVX2 0
#endif

namespace fast_compressor {
namespace fast_math {

constexpr float kSqrt2 = 1.41421356f;
constexpr float kLog2E = 1.44269504f;  // 1 / ln 2
constexpr float kLn2 = 0.693147180f;
constexpr float kExp2Min = -126.0f;
constexpr float kExp2Max = 126.0f;

// log2(x) = e + log2(m) for x = 2^e * m. The mantissa is centred on 1
// (m in [sqrt(1/2), sqrt(2))) and log2(m) comes from
// ln(m) = 2 atanh(s), s = (m - 1) / (m + 1), |s| < 0.172, whose odd series
// through s^7 is accurate to about 3e-8. Exact at powers of two.
inline float FastLog2(float x) {
  const uint32_t bits = std::bit_cast<uint32_t>(x);
  float e = static_cast<float>(static_cast<int32_t>(bits >> 23) - 127);
  float m = std::bit_cast<float>((bits & 0x007FFFFFu) | 0x3F800000u);
  if (m > kSqrt2) {
    m *= 0.5f;
    e += 1.0f;
  }
  const float s = (m - 1.0f) / (m + 1.0f);
  const float s2 = s * s;
  float p = 1.0f / 7.0f;
  p = p * s2 + 1.0f / 5.0f;
  p = p * s2 + 1.0f / 3.0f;
  p = p * s2 + 1.0f;
  const float ln_m = 2.0f * s * p;
  return ln_m * kLog2E + e;
}

// 2^x = 2^n * e^(f ln 2) for n = nearest integer, f in [-0.5, 0.5]. The
// exponential of y = f ln 2 (|y| < 0.347) is its Taylor series through y^6,
// accurate to about 1.2e-7; 2^n is built directly in the exponent field.
// Exact at integers.
inline float FastExp2(float x) {
  x = std::clamp(x, kExp2Min, kExp2Max);
  const float n = std::nearbyint(x);
  const float y = (x - n) * kLn2;
  float p = 1.0f / 720.0f;
  p = p * y + 1.0f / 120.0f;
  p = p * y + 1.0f / 24.0f;
  p = p * y + 1.0f / 6.0f;
  p = p * y + 0.5f;
  p = p * y + 1.0f;
  p = p * y + 1.0f;
  const int32_t scale_bits = (static_cast<int32_t>(n) + 127) << 23;
  return p * std::bit_cast<float>(scale_bits);
}

#if COMPRESSOR_FAST_MATH_AVX2

inline __m256 FastLog2(__m256 x) {
  const __m256i bits = _mm256_castps_si256(x);
  __m256 e = _mm256_cvtepi32_ps(
      _mm256_sub_epi32(_mm256_srli_epi32(bits, 23), _mm256_set1_epi32(127)));
  __m256 m = _mm256_castsi256_ps(
      _mm256_or_si256(_mm256_and_si256(bits, _mm256_set1_epi32(0x007FFFFF)),
                      _mm256_set1_epi32(0x3F800000)));
  const __m256 one = _mm256_set1_ps(1.0f);
  const __m256 big = _mm256_cmp_ps(m, _mm256_set1_ps(kSqrt2), _CMP_GT_OQ);
  m = _mm256_blendv_ps(m, _mm256_mul_ps(m, _mm256_set1_ps(0.5f)), big);
  e = _mm256_add_ps(e, _mm256_and_ps(big, one));
  const __m256 s =
      _mm256_div_ps(_mm256_sub_ps(m, one), _mm256_add_ps(m, one));
  const __m256 s2 = _mm256_mul_ps(s, s);
  __m256 p = _mm256_set1_ps(1.0f / 7.0f);
  p = _mm256_fmadd_ps(p, s2, _mm256_set1_ps(1.0f / 5.0f));
  p = _mm256_fmadd_ps(p, s2, _mm256_set1_ps(1.0f / 3.0f));
  p = _mm256_fmadd_ps(p, s2, one);
  const __m256 ln_m = _mm256_mul_ps(_mm256_mul_ps(_mm256_set1_ps(2.0f), s), p);
  return _mm256_fmadd_ps(ln_m, _mm256_set1_ps(kLog2E), e);
}

inline __m256 FastExp2(__m256 x) {
  x = _mm256_min_ps(_mm256_max_ps(x, _mm256_set1_ps(kExp2Min)),
                    _mm256_set1_ps(kExp2Max));
  const __m256 n =
      _mm256_round_ps(x, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
  const __m256 y = _mm256_mul_ps(_mm256_sub_ps(x, n), _mm256_set1_ps(kLn2));
  const __m256 one = _mm256_set1_ps(1.0f);
  __m256 p = _mm256_set1_ps(1.0f / 720.0f);
  p = _mm256_fmadd_ps(p, y, _mm256_set1_ps(1.0f / 120.0f));
  p = _mm256_fmadd_ps(p, y, _mm256_set1_ps(1.0f / 24.0f));
  p = _mm256_fmadd_ps(p, y, _mm256_set1_ps(1.0f / 6.0f));
  p = _mm256_fmadd_ps(p, y, _mm256_set1_ps(0.5f));
  p = _mm256_fmadd_ps(p, y, one);
  p = _mm256_fmadd_ps(p, y, one);
  const __m256i scale_bits = _mm256_slli_epi32(
      _mm256_add_epi32(_mm256_cvtps_epi32(n), _mm256_set1_epi32(127)), 23);
  return _mm256_mul_ps(p, _mm256_castsi256_ps(scale_bits));
}

#endif  // COMPRESSOR_FAST_MATH_AVX2

}  // namespace fast_math
}  // namespace fast_compressor

#endif  // COMPRESSOR_FAST_MATH_H_
