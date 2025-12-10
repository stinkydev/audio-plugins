// Copyright 2025
// SIMD Utilities Implementation

#include "simd_utils.h"

#include <algorithm>
#include <cmath>

#ifdef USE_SIMD
#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
#include <cpuid.h>
#include <immintrin.h>
#endif
#endif

namespace fast_compressor {
namespace simd {

namespace {

#ifdef USE_SIMD
bool g_simd_available = false;
bool g_simd_checked = false;

void CheckSimdSupport() {
  if (g_simd_checked) return;
  
  g_simd_checked = true;
  
#if defined(_MSC_VER)
  int cpu_info[4];
  __cpuid(cpu_info, 0);
  int num_ids = cpu_info[0];
  
  if (num_ids >= 7) {
    __cpuidex(cpu_info, 7, 0);
    // Check for AVX2 support (EBX bit 5)
    g_simd_available = (cpu_info[1] & (1 << 5)) != 0;
  }
#elif defined(__GNUC__) || defined(__clang__)
  unsigned int eax, ebx, ecx, edx;
  if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
    // Check for AVX2 support (EBX bit 5)
    g_simd_available = (ebx & (1 << 5)) != 0;
  }
#endif
}
#endif

}  // namespace

bool IsSimdAvailable() {
#ifdef USE_SIMD
  CheckSimdSupport();
  return g_simd_available;
#else
  return false;
#endif
}

void MultiplyAdd(float* dest, const float* src, float multiplier, 
                 size_t count) {
#ifdef USE_SIMD
  CheckSimdSupport();
  if (g_simd_available && count >= 8) {
    const size_t simd_count = count & ~7;  // Round down to multiple of 8
    const __m256 mult_vec = _mm256_set1_ps(multiplier);
    
    for (size_t i = 0; i < simd_count; i += 8) {
      __m256 src_vec = _mm256_loadu_ps(&src[i]);
      __m256 dest_vec = _mm256_loadu_ps(&dest[i]);
      dest_vec = _mm256_fmadd_ps(src_vec, mult_vec, dest_vec);
      _mm256_storeu_ps(&dest[i], dest_vec);
    }
    
    // Handle remaining elements
    for (size_t i = simd_count; i < count; ++i) {
      dest[i] += src[i] * multiplier;
    }
    return;
  }
#endif
  
  // Scalar fallback
  for (size_t i = 0; i < count; ++i) {
    dest[i] += src[i] * multiplier;
  }
}

void Multiply(float* dest, const float* src, float multiplier, size_t count) {
#ifdef USE_SIMD
  CheckSimdSupport();
  if (g_simd_available && count >= 8) {
    const size_t simd_count = count & ~7;
    const __m256 mult_vec = _mm256_set1_ps(multiplier);
    
    for (size_t i = 0; i < simd_count; i += 8) {
      __m256 src_vec = _mm256_loadu_ps(&src[i]);
      __m256 result = _mm256_mul_ps(src_vec, mult_vec);
      _mm256_storeu_ps(&dest[i], result);
    }
    
    for (size_t i = simd_count; i < count; ++i) {
      dest[i] = src[i] * multiplier;
    }
    return;
  }
#endif
  
  for (size_t i = 0; i < count; ++i) {
    dest[i] = src[i] * multiplier;
  }
}

void ApplyGain(float* buffer, float gain, size_t count) {
#ifdef USE_SIMD
  CheckSimdSupport();
  if (g_simd_available && count >= 8) {
    const size_t simd_count = count & ~7;
    const __m256 gain_vec = _mm256_set1_ps(gain);
    
    for (size_t i = 0; i < simd_count; i += 8) {
      __m256 buf_vec = _mm256_loadu_ps(&buffer[i]);
      buf_vec = _mm256_mul_ps(buf_vec, gain_vec);
      _mm256_storeu_ps(&buffer[i], buf_vec);
    }
    
    for (size_t i = simd_count; i < count; ++i) {
      buffer[i] *= gain;
    }
    return;
  }
#endif
  
  for (size_t i = 0; i < count; ++i) {
    buffer[i] *= gain;
  }
}

void ConvertToDb(float* dest, const float* src, size_t count) {
  constexpr float kEpsilon = 1e-8f;
  
  // This operation is difficult to vectorize efficiently due to log10
  // Use scalar implementation
  for (size_t i = 0; i < count; ++i) {
    const float abs_val = std::max(std::abs(src[i]), kEpsilon);
    dest[i] = 20.0f * std::log10f(abs_val);
  }
}

void Max(float* dest, const float* src1, const float* src2, size_t count) {
#ifdef USE_SIMD
  CheckSimdSupport();
  if (g_simd_available && count >= 8) {
    const size_t simd_count = count & ~7;
    
    for (size_t i = 0; i < simd_count; i += 8) {
      __m256 vec1 = _mm256_loadu_ps(&src1[i]);
      __m256 vec2 = _mm256_loadu_ps(&src2[i]);
      __m256 result = _mm256_max_ps(vec1, vec2);
      _mm256_storeu_ps(&dest[i], result);
    }
    
    for (size_t i = simd_count; i < count; ++i) {
      dest[i] = std::max(src1[i], src2[i]);
    }
    return;
  }
#endif
  
  for (size_t i = 0; i < count; ++i) {
    dest[i] = std::max(src1[i], src2[i]);
  }
}

void Min(float* dest, const float* src1, const float* src2, size_t count) {
#ifdef USE_SIMD
  CheckSimdSupport();
  if (g_simd_available && count >= 8) {
    const size_t simd_count = count & ~7;
    
    for (size_t i = 0; i < simd_count; i += 8) {
      __m256 vec1 = _mm256_loadu_ps(&src1[i]);
      __m256 vec2 = _mm256_loadu_ps(&src2[i]);
      __m256 result = _mm256_min_ps(vec1, vec2);
      _mm256_storeu_ps(&dest[i], result);
    }
    
    for (size_t i = simd_count; i < count; ++i) {
      dest[i] = std::min(src1[i], src2[i]);
    }
    return;
  }
#endif
  
  for (size_t i = 0; i < count; ++i) {
    dest[i] = std::min(src1[i], src2[i]);
  }
}

}  // namespace simd
}  // namespace fast_compressor
