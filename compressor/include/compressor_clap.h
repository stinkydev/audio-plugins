// Copyright 2025
// CLAP Plugin Wrapper for Fast Compressor

#ifndef COMPRESSOR_CLAP_H_
#define COMPRESSOR_CLAP_H_

#include <clap/clap.h>

#include <atomic>
#include <memory>

#include "compressor_processor.h"

namespace fast_compressor {

// @ts-plugin-meta
// name: Compressor
// id: com.stinky.compressor
// filename: StinkyCompressor.clap
// description: High-performance audio compressor with SIMD optimization

// @ts-port type=input id=0 name="Audio Input" channels=2 main=true
// @ts-port type=input id=1 name="Sidechain Input" channels=2 main=false
// @ts-port type=output id=0 name="Audio Output" channels=2 main=true

// Parameter ranges (actual units)
constexpr double kThresholdMin = -60.0;
constexpr double kThresholdMax = 0.0;
constexpr double kRatioMin = 1.0;
constexpr double kRatioMax = 100.0;
constexpr double kAttackMin = 0.05;
constexpr double kAttackMax = 250.0;
constexpr double kReleaseMin = 10.0;
constexpr double kReleaseMax = 2500.0;
constexpr double kKneeMin = 0.0;
constexpr double kKneeMax = 12.0;
constexpr double kMakeupMin = -12.0;
constexpr double kMakeupMax = 24.0;

// CLAP parameter IDs
enum CompressorParamId {
  kParamIdThreshold = 0,  // @ts-param min=-60.0 max=0.0 default=-20.0 unit=dB label="Threshold"
  kParamIdRatio,            // @ts-param min=1.0 max=100.0 default=4.0 unit=:1 label="Ratio" scale=log
  kParamIdAttack,           // @ts-param min=0.05 max=250.0 default=5.0 unit=ms label="Attack" scale=log
  kParamIdRelease,          // @ts-param min=10.0 max=2500.0 default=50.0 unit=ms label="Release" scale=log
  kParamIdKnee,             // @ts-param min=0.0 max=12.0 default=0.0 unit=dB label="Knee"
  kParamIdMakeupGain,       // @ts-param min=-12.0 max=24.0 default=0.0 unit=dB label="Makeup Gain"
  kParamIdAutoMakeup,       // @ts-param default=0 label="Auto Makeup" type=bool
  kParamIdCount
};

class CompressorClap {
 public:
  explicit CompressorClap(const clap_host_t* host);
  ~CompressorClap() = default;

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

 private:
  void ProcessParameterChanges(const clap_input_events_t* events) noexcept;
  void UpdateProcessorParams() noexcept;
  double ParamIdToValue(clap_id param_id) const noexcept;
  void SetParamValue(clap_id param_id, double value) noexcept;

  clap_plugin_t plugin_;
  const clap_host_t* host_;
  CompressorProcessor processor_;
  
  std::atomic<double> param_values_[kParamIdCount];
  double sample_rate_;
  bool is_processing_;
};

}  // namespace fast_compressor

#endif  // COMPRESSOR_CLAP_H_
