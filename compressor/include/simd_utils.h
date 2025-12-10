// Copyright 2025
// SIMD Utilities with Scalar Fallback

#ifndef SIMD_UTILS_H_
#define SIMD_UTILS_H_

#include <cstddef>

namespace fast_compressor {
namespace simd {

// Check if SIMD is available at runtime
bool IsSimdAvailable();

// Vector operations with SIMD acceleration and scalar fallback
void MultiplyAdd(float* dest, const float* src, float multiplier,
                 size_t count);

void Multiply(float* dest, const float* src, float multiplier, size_t count);

void ApplyGain(float* buffer, float gain, size_t count);

void ConvertToDb(float* dest, const float* src, size_t count);

void Max(float* dest, const float* src1, const float* src2, size_t count);

void Min(float* dest, const float* src1, const float* src2, size_t count);

}  // namespace simd
}  // namespace fast_compressor

#endif  // SIMD_UTILS_H_
