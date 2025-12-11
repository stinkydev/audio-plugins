// Copyright 2025
// CLAP Plugin Wrapper Header for Delay

#ifndef DELAY_CLAP_H_
#define DELAY_CLAP_H_

#include <clap/clap.h>

#include <atomic>
#include <memory>

#include "delay_processor.h"

namespace stinky_delay {

// @ts-plugin-meta
// name: Delay
// id: com.stinky.delay
// filename: StinkyDelay.clap
// description: Simple stereo delay effect

// Parameter ranges (actual units)
constexpr double kDelayTimeMin = 0.0;
constexpr double kDelayTimeMax = 1000.0;
constexpr double kMixMin = 0.0;
constexpr double kMixMax = 1.0;

enum DelayParamId {
  kParamIdDelayTime = 100,  // @ts-param min=0.0 max=1000.0 default=0.0 unit=ms label="Delay Time"
  kParamIdMix,              // @ts-param min=0.0 max=1.0 default=1.0 unit=% label="Mix"
  kParamIdCount
};

class DelayClap {
 public:
  explicit DelayClap(const clap_host_t* host);
  ~DelayClap() = default;

  bool Init() noexcept;
  bool Activate(double sample_rate, uint32_t min_frames,
                uint32_t max_frames) noexcept;
  void Deactivate() noexcept;
  bool StartProcessing() noexcept;
  void StopProcessing() noexcept;
  void Reset() noexcept;
  clap_process_status Process(const clap_process_t* process) noexcept;

  const void* GetExtension(const char* id) noexcept;

  clap_plugin_t* MutableClapPlugin() noexcept { return &plugin_; }

  uint32_t ParamsCount() const noexcept;
  bool ParamsInfo(uint32_t param_index, clap_param_info_t* info) const noexcept;
  bool ParamsValue(clap_id param_id, double* value) noexcept;
  bool ParamsValueToText(clap_id param_id, double value, char* display,
                         uint32_t size) noexcept;
  bool ParamsTextToValue(clap_id param_id, const char* display,
                         double* value) noexcept;
  void ParamsFlush(const clap_input_events_t* in,
                   const clap_output_events_t* out) noexcept;

  bool StateSave(const clap_ostream_t* stream) noexcept;
  bool StateLoad(const clap_istream_t* stream) noexcept;

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
  DelayProcessor processor_;
  
  std::atomic<double> param_values_[kParamIdCount];
  double sample_rate_;
  bool is_processing_;
};

}  // namespace stinky_delay

#endif  // DELAY_CLAP_H_
