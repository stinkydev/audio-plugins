// Copyright 2025
// CLAP Plugin Wrapper for 4-Band EQ

#ifndef EQ_CLAP_H_
#define EQ_CLAP_H_

#include <clap/clap.h>

#include <atomic>
#include <memory>

#include "eq_processor.h"

namespace fast_eq {

// @ts-plugin-meta
// name: Eq
// id: com.stinky.eq
// filename: SesameEQ.clap
// description: High-quality 4-band parametric EQ with multiple filter types

// Parameter ranges
constexpr double kFreqMin = 20.0;
constexpr double kFreqMax = 20000.0;
constexpr double kGainMin = -24.0;
constexpr double kGainMax = 24.0;
constexpr double kQMin = 0.1;
constexpr double kQMax = 10.0;
constexpr double kOutputGainMin = -12.0;
constexpr double kOutputGainMax = 12.0;

// CLAP parameter IDs
enum EqParamId {
  // Band 1
  kParamIdBand1Type = 100,  // @ts-param default=1 label="Band 1 Type" type=enum values="Low Cut,Bell,Low Shelf,High Shelf,High Cut"
  kParamIdBand1Freq,        // @ts-param min=20.0 max=20000.0 default=100.0 unit=Hz label="Band 1 Frequency" scale=log
  kParamIdBand1Gain,        // @ts-param min=-24.0 max=24.0 default=0.0 unit=dB label="Band 1 Gain"
  kParamIdBand1Q,           // @ts-param min=0.1 max=10.0 default=0.707 label="Band 1 Q"
  kParamIdBand1Enable,      // @ts-param default=1 label="Band 1 Enable" type=bool
  
  // Band 2
  kParamIdBand2Type,        // @ts-param default=1 label="Band 2 Type" type=enum values="Low Cut,Bell,Low Shelf,High Shelf,High Cut"
  kParamIdBand2Freq,        // @ts-param min=20.0 max=20000.0 default=500.0 unit=Hz label="Band 2 Frequency" scale=log
  kParamIdBand2Gain,        // @ts-param min=-24.0 max=24.0 default=0.0 unit=dB label="Band 2 Gain"
  kParamIdBand2Q,           // @ts-param min=0.1 max=10.0 default=1.0 label="Band 2 Q"
  kParamIdBand2Enable,      // @ts-param default=1 label="Band 2 Enable" type=bool
  
  // Band 3
  kParamIdBand3Type,        // @ts-param default=1 label="Band 3 Type" type=enum values="Low Cut,Bell,Low Shelf,High Shelf,High Cut"
  kParamIdBand3Freq,        // @ts-param min=20.0 max=20000.0 default=2000.0 unit=Hz label="Band 3 Frequency" scale=log
  kParamIdBand3Gain,        // @ts-param min=-24.0 max=24.0 default=0.0 unit=dB label="Band 3 Gain"
  kParamIdBand3Q,           // @ts-param min=0.1 max=10.0 default=1.0 label="Band 3 Q"
  kParamIdBand3Enable,      // @ts-param default=1 label="Band 3 Enable" type=bool
  
  // Band 4
  kParamIdBand4Type,        // @ts-param default=3 label="Band 4 Type" type=enum values="Low Cut,Bell,Low Shelf,High Shelf,High Cut"
  kParamIdBand4Freq,        // @ts-param min=20.0 max=20000.0 default=8000.0 unit=Hz label="Band 4 Frequency" scale=log
  kParamIdBand4Gain,        // @ts-param min=-24.0 max=24.0 default=0.0 unit=dB label="Band 4 Gain"
  kParamIdBand4Q,           // @ts-param min=0.1 max=10.0 default=0.707 label="Band 4 Q"
  kParamIdBand4Enable,      // @ts-param default=1 label="Band 4 Enable" type=bool
  
  // Global
  kParamIdOutputGain,       // @ts-param min=-12.0 max=12.0 default=0.0 unit=dB label="Output Gain"
  kParamIdBypass,           // @ts-param default=0 label="Bypass" type=bool
  
  kParamIdCount
};

class EqClap {
 public:
  explicit EqClap(const clap_host_t* host);
  ~EqClap() = default;

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
  EqProcessor processor_;
  
  std::atomic<double> param_values_[kParamIdCount];
  double sample_rate_;
  bool is_processing_;
};

}  // namespace fast_eq

#endif  // EQ_CLAP_H_
