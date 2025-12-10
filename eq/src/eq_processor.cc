// Copyright 2025
// 4-Band EQ - DSP Processing Implementation

#include "eq_processor.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace fast_eq {

namespace {

constexpr float kEpsilon = 1e-8f;
constexpr float kPi = std::numbers::pi_v<float>;

inline float DbToLinear(float db) {
  return std::pow(10.0f, db / 20.0f);
}

}  // namespace

// BiquadFilter implementation
BiquadFilter::BiquadFilter()
    : b0_(1.0), b1_(0.0), b2_(0.0),
      a1_(0.0), a2_(0.0),
      x1_(0.0), x2_(0.0),
      y1_(0.0), y2_(0.0) {}

void BiquadFilter::SetCoefficients(double b0, double b1, double b2,
                                   double a0, double a1, double a2) {
  // Normalize coefficients by a0
  b0_ = b0 / a0;
  b1_ = b1 / a0;
  b2_ = b2 / a0;
  a1_ = a1 / a0;
  a2_ = a2 / a0;
}

void BiquadFilter::SetHighCut(double frequency, double q, double sample_rate) {
  const double omega = 2.0 * kPi * frequency / sample_rate;
  const double cos_omega = std::cos(omega);
  const double sin_omega = std::sin(omega);
  const double alpha = sin_omega / (2.0 * q);

  const double b0 = (1.0 - cos_omega) / 2.0;
  const double b1 = 1.0 - cos_omega;
  const double b2 = (1.0 - cos_omega) / 2.0;
  const double a0 = 1.0 + alpha;
  const double a1 = -2.0 * cos_omega;
  const double a2 = 1.0 - alpha;

  SetCoefficients(b0, b1, b2, a0, a1, a2);
}

void BiquadFilter::SetLowCut(double frequency, double q, double sample_rate) {
  const double omega = 2.0 * kPi * frequency / sample_rate;
  const double cos_omega = std::cos(omega);
  const double sin_omega = std::sin(omega);
  const double alpha = sin_omega / (2.0 * q);

  const double b0 = (1.0 + cos_omega) / 2.0;
  const double b1 = -(1.0 + cos_omega);
  const double b2 = (1.0 + cos_omega) / 2.0;
  const double a0 = 1.0 + alpha;
  const double a1 = -2.0 * cos_omega;
  const double a2 = 1.0 - alpha;

  SetCoefficients(b0, b1, b2, a0, a1, a2);
}

void BiquadFilter::SetLowShelf(double frequency, double gain_db, double q, 
                                double sample_rate) {
  const double A = std::pow(10.0, gain_db / 40.0);
  const double omega = 2.0 * kPi * frequency / sample_rate;
  const double cos_omega = std::cos(omega);
  const double sin_omega = std::sin(omega);
  const double beta = std::sqrt(A) / q;

  const double b0 = A * ((A + 1.0) - (A - 1.0) * cos_omega + beta * sin_omega);
  const double b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cos_omega);
  const double b2 = A * ((A + 1.0) - (A - 1.0) * cos_omega - beta * sin_omega);
  const double a0 = (A + 1.0) + (A - 1.0) * cos_omega + beta * sin_omega;
  const double a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cos_omega);
  const double a2 = (A + 1.0) + (A - 1.0) * cos_omega - beta * sin_omega;

  SetCoefficients(b0, b1, b2, a0, a1, a2);
}

void BiquadFilter::SetHighShelf(double frequency, double gain_db, double q,
                                 double sample_rate) {
  const double A = std::pow(10.0, gain_db / 40.0);
  const double omega = 2.0 * kPi * frequency / sample_rate;
  const double cos_omega = std::cos(omega);
  const double sin_omega = std::sin(omega);
  const double beta = std::sqrt(A) / q;

  const double b0 = A * ((A + 1.0) + (A - 1.0) * cos_omega + beta * sin_omega);
  const double b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cos_omega);
  const double b2 = A * ((A + 1.0) + (A - 1.0) * cos_omega - beta * sin_omega);
  const double a0 = (A + 1.0) - (A - 1.0) * cos_omega + beta * sin_omega;
  const double a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cos_omega);
  const double a2 = (A + 1.0) - (A - 1.0) * cos_omega - beta * sin_omega;

  SetCoefficients(b0, b1, b2, a0, a1, a2);
}

void BiquadFilter::SetBell(double frequency, double gain_db, double q,
                           double sample_rate) {
  // Bell EQ (peaking filter), RBJ Audio EQ Cookbook
  const double A = std::pow(10.0, gain_db / 40.0);
  const double omega = 2.0 * kPi * frequency / sample_rate;
  const double cos_omega = std::cos(omega);
  const double sin_omega = std::sin(omega);
  const double alpha = sin_omega / (2.0 * q);

  const double b0 = 1.0 + alpha * A;
  const double b1 = -2.0 * cos_omega;
  const double b2 = 1.0 - alpha * A;
  const double a0 = 1.0 + alpha / A;
  const double a1 = -2.0 * cos_omega;
  const double a2 = 1.0 - alpha / A;

  SetCoefficients(b0, b1, b2, a0, a1, a2);
}

float BiquadFilter::Process(float input) {
  const double output = b0_ * input + b1_ * x1_ + b2_ * x2_
                        - a1_ * y1_ - a2_ * y2_;
  
  // Update delay lines
  x2_ = x1_;
  x1_ = input;
  y2_ = y1_;
  y1_ = output;
  
  return static_cast<float>(output);
}

void BiquadFilter::Reset() {
  x1_ = x2_ = 0.0;
  y1_ = y2_ = 0.0;
}

// EqProcessor implementation
EqProcessor::EqProcessor()
    : sample_rate_(44100.0) {
  // Initialize default band parameters
  params_.bands[0].type = FilterType::kLowShelf;
  params_.bands[0].frequency_hz = 100.0f;
  params_.bands[0].gain_db = 0.0f;
  params_.bands[0].q = 0.707f;
  
  params_.bands[1].type = FilterType::kBell;
  params_.bands[1].frequency_hz = 500.0f;
  params_.bands[1].gain_db = 0.0f;
  params_.bands[1].q = 1.0f;
  
  params_.bands[2].type = FilterType::kBell;
  params_.bands[2].frequency_hz = 2000.0f;
  params_.bands[2].gain_db = 0.0f;
  params_.bands[2].q = 1.0f;
  
  params_.bands[3].type = FilterType::kHighShelf;
  params_.bands[3].frequency_hz = 8000.0f;
  params_.bands[3].gain_db = 0.0f;
  params_.bands[3].q = 0.707f;
}

void EqProcessor::Initialize(double sample_rate) {
  sample_rate_ = sample_rate;
  Reset();
  
  // Update all filter coefficients
  for (size_t i = 0; i < 4; ++i) {
    UpdateBandCoefficients(i);
  }
}

void EqProcessor::SetParams(const EqParams& params) {
  params_ = params;
  
  // Update filter coefficients for all bands
  for (size_t i = 0; i < 4; ++i) {
    UpdateBandCoefficients(i);
  }
}

void EqProcessor::UpdateBandCoefficients(size_t band_index) {
  const auto& band = params_.bands[band_index];
  
  switch (band.type) {
    case FilterType::kHighCut:
      filters_left_[band_index].SetHighCut(band.frequency_hz, band.q, sample_rate_);
      filters_right_[band_index].SetHighCut(band.frequency_hz, band.q, sample_rate_);
      break;
      
    case FilterType::kLowCut:
      filters_left_[band_index].SetLowCut(band.frequency_hz, band.q, sample_rate_);
      filters_right_[band_index].SetLowCut(band.frequency_hz, band.q, sample_rate_);
      break;
      
    case FilterType::kLowShelf:
      filters_left_[band_index].SetLowShelf(band.frequency_hz, band.gain_db, 
                                            band.q, sample_rate_);
      filters_right_[band_index].SetLowShelf(band.frequency_hz, band.gain_db,
                                             band.q, sample_rate_);
      break;
      
    case FilterType::kHighShelf:
      filters_left_[band_index].SetHighShelf(band.frequency_hz, band.gain_db,
                                             band.q, sample_rate_);
      filters_right_[band_index].SetHighShelf(band.frequency_hz, band.gain_db,
                                              band.q, sample_rate_);
      break;
      
    case FilterType::kBell:
      filters_left_[band_index].SetBell(band.frequency_hz, band.gain_db,
                                        band.q, sample_rate_);
      filters_right_[band_index].SetBell(band.frequency_hz, band.gain_db,
                                         band.q, sample_rate_);
      break;
  }
}

void EqProcessor::Reset() {
  for (auto& filter : filters_left_) {
    filter.Reset();
  }
  for (auto& filter : filters_right_) {
    filter.Reset();
  }
}

void EqProcessor::Process(float* buffer, size_t num_frames) {
  if (params_.bypass) {
    return;
  }
  
  const float output_gain = DbToLinear(params_.output_gain_db);
  
  for (size_t i = 0; i < num_frames * 2; i += 2) {
    float left = buffer[i];
    float right = buffer[i + 1];
    
    // Process each enabled band
    for (size_t band = 0; band < 4; ++band) {
      if (params_.bands[band].enabled) {
        left = filters_left_[band].Process(left);
        right = filters_right_[band].Process(right);
      }
    }
    
    // Apply output gain
    buffer[i] = left * output_gain;
    buffer[i + 1] = right * output_gain;
  }
}

void EqProcessor::ProcessStereo(float* left, float* right, size_t num_frames) {
  if (params_.bypass) {
    return;
  }
  
  const float output_gain = DbToLinear(params_.output_gain_db);
  
  for (size_t i = 0; i < num_frames; ++i) {
    float l = left[i];
    float r = right[i];
    
    // Process each enabled band
    for (size_t band = 0; band < 4; ++band) {
      if (params_.bands[band].enabled) {
        l = filters_left_[band].Process(l);
        r = filters_right_[band].Process(r);
      }
    }
    
    // Apply output gain
    left[i] = l * output_gain;
    right[i] = r * output_gain;
  }
}

}  // namespace fast_eq
