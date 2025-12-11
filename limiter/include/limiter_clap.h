// Copyright 2025
// CLAP Plugin Wrapper for Fast Limiter

#ifndef LIMITER_CLAP_H_
#define LIMITER_CLAP_H_

#include <clap/clap.h>

#include <atomic>
#include <memory>

#include "limiter_processor.h"

namespace fast_limiter {

// @ts-plugin-meta
// name: Limiter
// id: com.stinky.limiter
// filename: StinkyLimiter.clap
// description: High-performance peak limiter with lookahead and SIMD optimization

// Parameter ranges
constexpr double kThresholdMin = -60.0;
constexpr double kThresholdMax = 0.0;
constexpr double kOutputLevelMin = -60.0;
constexpr double kOutputLevelMax = 0.0;

// CLAP parameter IDs
enum LimiterParamId {
  kParamIdThreshold = 100,    // @ts-param min=-60.0 max=0.0 default=-0.1 unit=dB label="Threshold"
  kParamIdOutputLevel,        // @ts-param min=-60.0 max=0.0 default=-0.1 unit=dB label="Output Level"
  kParamIdCount
};

class LimiterClap {
 public:
  explicit LimiterClap(const clap_host_t* host);
  ~LimiterClap() = default;

  // CLAP plugin interface
  bool Init() noexcept;
  bool Activate(double sample_rate, uint32_t min_frames,
                uint32_t max_frames) noexcept;
  void Deactivate() noexcept;
  bool StartProcessing() noexcept;
  void StopProcessing() noexcept;
  void Reset() noexcept;
  clap_process_status Process(const clap_process_t* process) noexcept;

  // Extensions
  const void* GetExtension(const char* id) noexcept;

  // Get mutable plugin pointer for descriptor initialization
  clap_plugin_t* MutableClapPlugin() noexcept { return &plugin_; }

  // Parameters extension
  uint32_t ParamsCount() const noexcept;
  bool ParamsInfo(uint32_t param_index, clap_param_info_t* info) const noexcept;
  bool ParamsValue(clap_id param_id, double* value) noexcept;
  bool ParamsValueToText(clap_id param_id, double value, char* display,
                         uint32_t size) noexcept;
  bool ParamsTextToValue(clap_id param_id, const char* display,
                         double* value) noexcept;
  void ParamsFlush(const clap_input_events_t* in,
                   const clap_output_events_t* out) noexcept;

  // State extension
  bool StateSave(const clap_ostream_t* stream) noexcept;
  bool StateLoad(const clap_istream_t* stream) noexcept;

  // Audio ports extension
  uint32_t AudioPortsCount(bool is_input) const noexcept;
  bool AudioPortsGet(uint32_t index, bool is_input,
                     clap_audio_port_info_t* info) const noexcept;

  const clap_plugin_t* ClapPlugin() noexcept { return &plugin_; }

  // Get current gain reduction for metering
  float GetGainReduction() const noexcept { return processor_.GetGainReduction(); }

 private:
  void ProcessParameterChanges(const clap_input_events_t* events) noexcept;
  void UpdateProcessorParams() noexcept;
  double ParamIdToValue(clap_id param_id) const noexcept;
  void SetParamValue(clap_id param_id, double value) noexcept;

  clap_plugin_t plugin_;
  const clap_host_t* host_;
  LimiterProcessor processor_;
  
  std::atomic<double> param_values_[kParamIdCount];
  double sample_rate_;
  bool is_processing_;
};

}  // namespace fast_limiter

#endif  // LIMITER_CLAP_H_
