// Copyright 2025
// CLAP Plugin Wrapper Implementation

#include "compressor_clap.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace fast_compressor {

namespace {

constexpr const char* kPluginId = "com.stinky.compressor";
constexpr const char* kPluginName = "Compressor";
constexpr const char* kPluginVendor = "Stinky";
constexpr const char* kPluginUrl = "https://github.com/stinkydev/audio-plugins";
constexpr const char* kPluginVersion = "1.0.0";
constexpr const char* kPluginDescription = 
    "High-performance audio compressor with SIMD optimization";

constexpr const char* kFeatures[] = {
    CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
    CLAP_PLUGIN_FEATURE_COMPRESSOR,
    CLAP_PLUGIN_FEATURE_STEREO,
    nullptr
};

// Convert normalized [0,1] to actual values
inline double NormalizedToThreshold(double norm) {
  return kThresholdMin + norm * (kThresholdMax - kThresholdMin);
}

inline double ThresholdToNormalized(double db) {
  return (db - kThresholdMin) / (kThresholdMax - kThresholdMin);
}

inline double NormalizedToRatio(double norm) {
  // Logarithmic scaling for ratio
  return kRatioMin * std::pow(kRatioMax / kRatioMin, norm);
}

inline double RatioToNormalized(double ratio) {
  return std::log(ratio / kRatioMin) / std::log(kRatioMax / kRatioMin);
}

inline double NormalizedToAttack(double norm) {
  // Logarithmic scaling for attack time
  return kAttackMin * std::pow(kAttackMax / kAttackMin, norm);
}

inline double AttackToNormalized(double ms) {
  return std::log(ms / kAttackMin) / std::log(kAttackMax / kAttackMin);
}

inline double NormalizedToRelease(double norm) {
  // Logarithmic scaling for release time
  return kReleaseMin * std::pow(kReleaseMax / kReleaseMin, norm);
}

inline double ReleaseToNormalized(double ms) {
  return std::log(ms / kReleaseMin) / std::log(kReleaseMax / kReleaseMin);
}

inline double NormalizedToKnee(double norm) {
  return kKneeMin + norm * (kKneeMax - kKneeMin);
}

inline double KneeToNormalized(double db) {
  return (db - kKneeMin) / (kKneeMax - kKneeMin);
}

inline double NormalizedToMakeup(double norm) {
  return kMakeupMin + norm * (kMakeupMax - kMakeupMin);
}

inline double MakeupToNormalized(double db) {
  return (db - kMakeupMin) / (kMakeupMax - kMakeupMin);
}

// CLAP plugin callbacks
bool ClapInit(const clap_plugin_t* plugin) {
  auto* comp = static_cast<CompressorClap*>(plugin->plugin_data);
  return comp->Init();
}

void ClapDestroy(const clap_plugin_t* plugin) {
  auto* comp = static_cast<CompressorClap*>(plugin->plugin_data);
  delete comp;
}

bool ClapActivate(const clap_plugin_t* plugin, double sample_rate,
                  uint32_t min_frames, uint32_t max_frames) {
  auto* comp = static_cast<CompressorClap*>(plugin->plugin_data);
  return comp->Activate(sample_rate, min_frames, max_frames);
}

void ClapDeactivate(const clap_plugin_t* plugin) {
  auto* comp = static_cast<CompressorClap*>(plugin->plugin_data);
  comp->Deactivate();
}

bool ClapStartProcessing(const clap_plugin_t* plugin) {
  auto* comp = static_cast<CompressorClap*>(plugin->plugin_data);
  return comp->StartProcessing();
}

void ClapStopProcessing(const clap_plugin_t* plugin) {
  auto* comp = static_cast<CompressorClap*>(plugin->plugin_data);
  comp->StopProcessing();
}

void ClapReset(const clap_plugin_t* plugin) {
  auto* comp = static_cast<CompressorClap*>(plugin->plugin_data);
  comp->Reset();
}

clap_process_status ClapProcess(const clap_plugin_t* plugin,
                                 const clap_process_t* process) {
  auto* comp = static_cast<CompressorClap*>(plugin->plugin_data);
  return comp->Process(process);
}

const void* ClapGetExtension(const clap_plugin_t* plugin, const char* id) {
  auto* comp = static_cast<CompressorClap*>(plugin->plugin_data);
  return comp->GetExtension(id);
}

void ClapOnMainThread(const clap_plugin_t*) {}

// Parameters extension callbacks
uint32_t ClapParamsCount(const clap_plugin_t* plugin) {
  auto* comp = static_cast<CompressorClap*>(plugin->plugin_data);
  return comp->ParamsCount();
}

bool ClapParamsGetInfo(const clap_plugin_t* plugin, uint32_t param_index,
                       clap_param_info_t* param_info) {
  auto* comp = static_cast<CompressorClap*>(plugin->plugin_data);
  return comp->ParamsInfo(param_index, param_info);
}

bool ClapParamsGetValue(const clap_plugin_t* plugin, clap_id param_id,
                        double* value) {
  auto* comp = static_cast<CompressorClap*>(plugin->plugin_data);
  return comp->ParamsValue(param_id, value);
}

bool ClapParamsValueToText(const clap_plugin_t* plugin, clap_id param_id,
                           double value, char* display, uint32_t size) {
  auto* comp = static_cast<CompressorClap*>(plugin->plugin_data);
  return comp->ParamsValueToText(param_id, value, display, size);
}

bool ClapParamsTextToValue(const clap_plugin_t* plugin, clap_id param_id,
                           const char* display, double* value) {
  auto* comp = static_cast<CompressorClap*>(plugin->plugin_data);
  return comp->ParamsTextToValue(param_id, display, value);
}

void ClapParamsFlush(const clap_plugin_t* plugin,
                     const clap_input_events_t* in,
                     const clap_output_events_t* out) {
  auto* comp = static_cast<CompressorClap*>(plugin->plugin_data);
  comp->ParamsFlush(in, out);
}

static const clap_plugin_params_t kParamsExtension = {
    ClapParamsCount,
    ClapParamsGetInfo,
    ClapParamsGetValue,
    ClapParamsValueToText,
    ClapParamsTextToValue,
    ClapParamsFlush,
};

// State extension callbacks
bool ClapStateSave(const clap_plugin_t* plugin, const clap_ostream_t* stream) {
  auto* comp = static_cast<CompressorClap*>(plugin->plugin_data);
  return comp->StateSave(stream);
}

bool ClapStateLoad(const clap_plugin_t* plugin, const clap_istream_t* stream) {
  auto* comp = static_cast<CompressorClap*>(plugin->plugin_data);
  return comp->StateLoad(stream);
}

static const clap_plugin_state_t kStateExtension = {
    ClapStateSave,
    ClapStateLoad,
};

// Audio ports extension callbacks
uint32_t ClapAudioPortsCount(const clap_plugin_t* plugin, bool is_input) {
  auto* comp = static_cast<CompressorClap*>(plugin->plugin_data);
  return comp->AudioPortsCount(is_input);
}

bool ClapAudioPortsGet(const clap_plugin_t* plugin, uint32_t index,
                       bool is_input, clap_audio_port_info_t* info) {
  auto* comp = static_cast<CompressorClap*>(plugin->plugin_data);
  return comp->AudioPortsGet(index, is_input, info);
}

static const clap_plugin_audio_ports_t kAudioPortsExtension = {
    ClapAudioPortsCount,
    ClapAudioPortsGet,
};

}  // namespace

CompressorClap::CompressorClap(const clap_host_t* host)
    : host_(host),
      sample_rate_(44100.0),
      is_processing_(false) {
  plugin_.desc = nullptr;  // Set by factory
  plugin_.plugin_data = this;
  plugin_.init = ClapInit;
  plugin_.destroy = ClapDestroy;
  plugin_.activate = ClapActivate;
  plugin_.deactivate = ClapDeactivate;
  plugin_.start_processing = ClapStartProcessing;
  plugin_.stop_processing = ClapStopProcessing;
  plugin_.reset = ClapReset;
  plugin_.process = ClapProcess;
  plugin_.get_extension = ClapGetExtension;
  plugin_.on_main_thread = ClapOnMainThread;

  // Initialize parameters to normalized defaults
  param_values_[kParamIdThreshold].store(ThresholdToNormalized(-20.0));
  param_values_[kParamIdRatio].store(RatioToNormalized(4.0));
  param_values_[kParamIdAttack].store(AttackToNormalized(5.0));
  param_values_[kParamIdRelease].store(ReleaseToNormalized(50.0));
  param_values_[kParamIdKnee].store(KneeToNormalized(0.0));
  param_values_[kParamIdMakeupGain].store(MakeupToNormalized(0.0));
  param_values_[kParamIdAutoMakeup].store(0.0);
}

bool CompressorClap::Init() noexcept {
  UpdateProcessorParams();
  return true;
}

bool CompressorClap::Activate(double sample_rate, uint32_t /*min_frames*/,
                               uint32_t /*max_frames*/) noexcept {
  sample_rate_ = sample_rate;
  processor_.Initialize(sample_rate);
  UpdateProcessorParams();  // Sync stored parameter values with processor
  return true;
}

void CompressorClap::Deactivate() noexcept {
  is_processing_ = false;
}

bool CompressorClap::StartProcessing() noexcept {
  is_processing_ = true;
  return true;
}

void CompressorClap::StopProcessing() noexcept {
  is_processing_ = false;
}

void CompressorClap::Reset() noexcept {
  processor_.Reset();
}

clap_process_status CompressorClap::Process(
    const clap_process_t* process) noexcept {
  // Process parameter changes
  if (process->in_events) {
    ProcessParameterChanges(process->in_events);
  }

  const uint32_t frame_count = process->frames_count;
  const uint32_t input_count = process->audio_inputs_count;
  const uint32_t output_count = process->audio_outputs_count;

  if (input_count == 0 || output_count == 0 || frame_count == 0) {
    return CLAP_PROCESS_SLEEP;
  }

  // Get input/output channels (handle both mono and stereo)
  const uint32_t in_channels = process->audio_inputs[0].channel_count;
  const uint32_t out_channels = process->audio_outputs[0].channel_count;
  
  float* in_left = process->audio_inputs[0].data32[0];
  float* in_right = (in_channels > 1) ? process->audio_inputs[0].data32[1] : nullptr;
  float* out_left = process->audio_outputs[0].data32[0];
  float* out_right = (out_channels > 1) ? process->audio_outputs[0].data32[1] : nullptr;

  // Check for sidechain input (second input port)
  const float* sc_left = nullptr;
  const float* sc_right = nullptr;
  if (input_count >= 2) {
    const uint32_t sc_channels = process->audio_inputs[1].channel_count;
    sc_left = process->audio_inputs[1].data32[0];
    sc_right = (sc_channels > 1) ? process->audio_inputs[1].data32[1] : sc_left;
  }

  // Copy input to output
  std::memcpy(out_left, in_left, frame_count * sizeof(float));
  if (in_right && out_right) {
    std::memcpy(out_right, in_right, frame_count * sizeof(float));
  }

  // Process compression
  if (out_right) {
    if (sc_left) {
      // Use sidechain for detection
      processor_.ProcessStereoWithSidechain(out_left, out_right, sc_left, sc_right, frame_count);
    } else {
      // Use main input for detection
      processor_.ProcessStereo(out_left, out_right, frame_count);
    }
  } else {
    if (sc_left) {
      processor_.ProcessStereoWithSidechain(out_left, out_left, sc_left, sc_right, frame_count);
    } else {
      processor_.ProcessStereo(out_left, out_left, frame_count);
    }
  }

  return CLAP_PROCESS_CONTINUE;
}

const void* CompressorClap::GetExtension(const char* id) noexcept {
  if (std::strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) {
    return &kAudioPortsExtension;
  }
  if (std::strcmp(id, CLAP_EXT_PARAMS) == 0) {
    return &kParamsExtension;
  }
  if (std::strcmp(id, CLAP_EXT_STATE) == 0) {
    return &kStateExtension;
  }
  return nullptr;
}

uint32_t CompressorClap::ParamsCount() const noexcept {
  return kParamIdCount;
}

bool CompressorClap::ParamsInfo(uint32_t param_index,
                                clap_param_info_t* info) const noexcept {
  if (param_index >= kParamIdCount) return false;

  info->id = param_index;
  info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE;
  info->cookie = nullptr;

  switch (param_index) {
    case kParamIdThreshold:
      std::snprintf(info->name, sizeof(info->name), "Threshold");
      std::snprintf(info->module, sizeof(info->module), "");
      info->min_value = 0.0;
      info->max_value = 1.0;
      info->default_value = ThresholdToNormalized(-20.0);
      break;
    case kParamIdRatio:
      std::snprintf(info->name, sizeof(info->name), "Ratio");
      std::snprintf(info->module, sizeof(info->module), "");
      info->min_value = 0.0;
      info->max_value = 1.0;
      info->default_value = RatioToNormalized(4.0);
      break;
    case kParamIdAttack:
      std::snprintf(info->name, sizeof(info->name), "Attack");
      std::snprintf(info->module, sizeof(info->module), "");
      info->min_value = 0.0;
      info->max_value = 1.0;
      info->default_value = AttackToNormalized(5.0);
      break;
    case kParamIdRelease:
      std::snprintf(info->name, sizeof(info->name), "Release");
      std::snprintf(info->module, sizeof(info->module), "");
      info->min_value = 0.0;
      info->max_value = 1.0;
      info->default_value = ReleaseToNormalized(50.0);
      break;
    case kParamIdKnee:
      std::snprintf(info->name, sizeof(info->name), "Knee");
      std::snprintf(info->module, sizeof(info->module), "");
      info->min_value = 0.0;
      info->max_value = 1.0;
      info->default_value = KneeToNormalized(0.0);
      break;
    case kParamIdMakeupGain:
      std::snprintf(info->name, sizeof(info->name), "Makeup Gain");
      std::snprintf(info->module, sizeof(info->module), "");
      info->min_value = 0.0;
      info->max_value = 1.0;
      info->default_value = MakeupToNormalized(0.0);
      break;
    case kParamIdAutoMakeup:
      std::snprintf(info->name, sizeof(info->name), "Auto Makeup");
      std::snprintf(info->module, sizeof(info->module), "");
      info->min_value = 0.0;
      info->max_value = 1.0;
      info->default_value = 0.0;
      info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_STEPPED;
      break;
    default:
      return false;
  }

  return true;
}

bool CompressorClap::ParamsValue(clap_id param_id, double* value) noexcept {
  if (param_id >= kParamIdCount) return false;
  *value = param_values_[param_id].load();
  return true;
}

bool CompressorClap::ParamsValueToText(clap_id param_id, double value,
                                       char* display,
                                       uint32_t size) noexcept {
  if (param_id >= kParamIdCount) return false;

  switch (param_id) {
    case kParamIdThreshold:
      std::snprintf(display, size, "%.1f dB", NormalizedToThreshold(value));
      break;
    case kParamIdKnee:
      std::snprintf(display, size, "%.1f dB", NormalizedToKnee(value));
      break;
    case kParamIdMakeupGain:
      std::snprintf(display, size, "%.1f dB", NormalizedToMakeup(value));
      break;
    case kParamIdRatio:
      std::snprintf(display, size, "%.1f:1", NormalizedToRatio(value));
      break;
    case kParamIdAttack:
      std::snprintf(display, size, "%.1f ms", NormalizedToAttack(value));
      break;
    case kParamIdRelease:
      std::snprintf(display, size, "%.1f ms", NormalizedToRelease(value));
      break;
    case kParamIdAutoMakeup:
      std::snprintf(display, size, "%s", value > 0.5 ? "On" : "Off");
      break;
    default:
      return false;
  }

  return true;
}

bool CompressorClap::ParamsTextToValue(clap_id param_id, const char* display,
                                       double* value) noexcept {
  if (param_id >= kParamIdCount) return false;
  
  double parsed_value;
#ifdef _MSC_VER
  if (sscanf_s(display, "%lf", &parsed_value) != 1) {
#else
  if (std::sscanf(display, "%lf", &parsed_value) != 1) {
#endif
    return false;
  }

  // Convert parsed actual value to normalized [0,1]
  switch (param_id) {
    case kParamIdThreshold:
      parsed_value = std::clamp(parsed_value, kThresholdMin, kThresholdMax);
      *value = ThresholdToNormalized(parsed_value);
      break;
    case kParamIdRatio:
      parsed_value = std::clamp(parsed_value, kRatioMin, kRatioMax);
      *value = RatioToNormalized(parsed_value);
      break;
    case kParamIdAttack:
      parsed_value = std::clamp(parsed_value, kAttackMin, kAttackMax);
      *value = AttackToNormalized(parsed_value);
      break;
    case kParamIdRelease:
      parsed_value = std::clamp(parsed_value, kReleaseMin, kReleaseMax);
      *value = ReleaseToNormalized(parsed_value);
      break;
    case kParamIdKnee:
      parsed_value = std::clamp(parsed_value, kKneeMin, kKneeMax);
      *value = KneeToNormalized(parsed_value);
      break;
    case kParamIdMakeupGain:
      parsed_value = std::clamp(parsed_value, kMakeupMin, kMakeupMax);
      *value = MakeupToNormalized(parsed_value);
      break;
    case kParamIdAutoMakeup:
      *value = std::clamp(parsed_value, 0.0, 1.0);
      break;
    default:
      return false;
  }

  return true;
}

void CompressorClap::ParamsFlush(const clap_input_events_t* in,
                                 const clap_output_events_t* /*out*/) noexcept {
  ProcessParameterChanges(in);
}

bool CompressorClap::StateSave(const clap_ostream_t* stream) noexcept {
  double values[kParamIdCount];
  for (int i = 0; i < kParamIdCount; ++i) {
    values[i] = param_values_[i].load();
  }
  
  int64_t written = stream->write(stream, values, sizeof(values));
  return written == sizeof(values);
}

bool CompressorClap::StateLoad(const clap_istream_t* stream) noexcept {
  double values[kParamIdCount];
  int64_t read = stream->read(stream, values, sizeof(values));
  
  if (read != sizeof(values)) return false;

  for (int i = 0; i < kParamIdCount; ++i) {
    param_values_[i].store(values[i]);
  }

  UpdateProcessorParams();
  return true;
}

void CompressorClap::ProcessParameterChanges(
    const clap_input_events_t* events) noexcept {
  const uint32_t event_count = events->size(events);

  for (uint32_t i = 0; i < event_count; ++i) {
    const clap_event_header_t* header = events->get(events, i);

    if (header->space_id != CLAP_CORE_EVENT_SPACE_ID) continue;

    if (header->type == CLAP_EVENT_PARAM_VALUE) {
      auto* param_event = reinterpret_cast<const clap_event_param_value_t*>(header);
      SetParamValue(param_event->param_id, param_event->value);
    }
  }

  UpdateProcessorParams();
}

void CompressorClap::UpdateProcessorParams() noexcept {
  CompressorParams params;
  params.threshold_db = static_cast<float>(NormalizedToThreshold(param_values_[kParamIdThreshold].load()));
  params.ratio = static_cast<float>(NormalizedToRatio(param_values_[kParamIdRatio].load()));
  params.attack_ms = static_cast<float>(NormalizedToAttack(param_values_[kParamIdAttack].load()));
  params.release_ms = static_cast<float>(NormalizedToRelease(param_values_[kParamIdRelease].load()));
  params.knee_db = static_cast<float>(NormalizedToKnee(param_values_[kParamIdKnee].load()));
  params.makeup_gain_db = static_cast<float>(NormalizedToMakeup(param_values_[kParamIdMakeupGain].load()));
  params.auto_makeup = param_values_[kParamIdAutoMakeup].load() > 0.5;
  
  processor_.SetParams(params);
}

void CompressorClap::SetParamValue(clap_id param_id, double value) noexcept {
  if (param_id < kParamIdCount) {
    param_values_[param_id].store(value);
  }
}

uint32_t CompressorClap::AudioPortsCount(bool is_input) const noexcept {
  return is_input ? 2 : 1;  // Two stereo inputs (main + sidechain), one output
}

bool CompressorClap::AudioPortsGet(uint32_t index, bool is_input,
                                   clap_audio_port_info_t* info) const noexcept {
  if (is_input) {
    if (index > 1) return false;
    
    info->id = index;
    if (index == 0) {
      std::snprintf(info->name, sizeof(info->name), "Audio Input");
      info->flags = CLAP_AUDIO_PORT_IS_MAIN;
      info->in_place_pair = 0;
    } else {
      std::snprintf(info->name, sizeof(info->name), "Sidechain Input");
      info->flags = 0;  // Not main, not required
      info->in_place_pair = CLAP_INVALID_ID;
    }
    info->channel_count = 2;
    info->port_type = CLAP_PORT_STEREO;
  } else {
    if (index > 0) return false;
    
    info->id = 0;
    std::snprintf(info->name, sizeof(info->name), "Audio Output");
    info->channel_count = 2;
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->port_type = CLAP_PORT_STEREO;
    info->in_place_pair = 0;
  }

  return true;
}

}  // namespace fast_compressor

// CLAP plugin factory
extern "C" {

static const clap_plugin_descriptor_t kDescriptor = {
    CLAP_VERSION,
    fast_compressor::kPluginId,
    fast_compressor::kPluginName,
    fast_compressor::kPluginVendor,
    fast_compressor::kPluginUrl,
    nullptr,
    nullptr,
    fast_compressor::kPluginVersion,
    fast_compressor::kPluginDescription,
    fast_compressor::kFeatures,
};

static const clap_plugin_t* ClapCreatePlugin(const clap_plugin_factory_t* /*factory*/,
                                              const clap_host_t* host,
                                              const char* plugin_id) {
  if (std::strcmp(plugin_id, fast_compressor::kPluginId) != 0) {
    return nullptr;
  }

  auto* plugin = new fast_compressor::CompressorClap(host);
  auto* clap_plugin = plugin->MutableClapPlugin();
  clap_plugin->desc = &kDescriptor;
  return clap_plugin;
}

static uint32_t ClapGetPluginCount(const clap_plugin_factory_t* /*factory*/) {
  return 1;
}

static const clap_plugin_descriptor_t* ClapGetPluginDescriptor(
    const clap_plugin_factory_t* /*factory*/, uint32_t index) {
  return (index == 0) ? &kDescriptor : nullptr;
}

static const clap_plugin_factory_t kFactory = {
    ClapGetPluginCount,
    ClapGetPluginDescriptor,
    ClapCreatePlugin,
};

static const void* ClapGetFactory(const char* factory_id) {
  return std::strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0 
      ? &kFactory : nullptr;
}

CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
    CLAP_VERSION,
    [](const char* /*plugin_path*/) { return true; },
    []() {},
    ClapGetFactory,
};

}  // extern "C"
