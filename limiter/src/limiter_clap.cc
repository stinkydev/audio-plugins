// Copyright 2025
// CLAP Plugin Wrapper Implementation

#include "limiter_clap.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace fast_limiter {

namespace {

constexpr const char* kPluginId = "com.stinky.limiter";
constexpr const char* kPluginName = "Limiter";
constexpr const char* kPluginVendor = "Stinky";
constexpr const char* kPluginUrl = "https://github.com/stinkydev/audio-plugins";
constexpr const char* kPluginVersion = "1.0.0";
constexpr const char* kPluginDescription = 
    "High-performance peak limiter with lookahead and SIMD optimization";

constexpr const char* kFeatures[] = {
    CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
    CLAP_PLUGIN_FEATURE_LIMITER,
    CLAP_PLUGIN_FEATURE_STEREO,
    nullptr
};

// Conversion helper functions
inline double NormalizedToThreshold(double norm) {
  return kThresholdMin + norm * (kThresholdMax - kThresholdMin);
}

inline double ThresholdToNormalized(double db) {
  return (db - kThresholdMin) / (kThresholdMax - kThresholdMin);
}

inline double NormalizedToOutputLevel(double norm) {
  return kOutputLevelMin + norm * (kOutputLevelMax - kOutputLevelMin);
}

inline double OutputLevelToNormalized(double db) {
  return (db - kOutputLevelMin) / (kOutputLevelMax - kOutputLevelMin);
}

// CLAP plugin callbacks
bool ClapInit(const clap_plugin_t* plugin) {
  auto* limiter = static_cast<LimiterClap*>(plugin->plugin_data);
  return limiter->Init();
}

void ClapDestroy(const clap_plugin_t* plugin) {
  auto* limiter = static_cast<LimiterClap*>(plugin->plugin_data);
  delete limiter;
}

bool ClapActivate(const clap_plugin_t* plugin, double sample_rate,
                  uint32_t min_frames, uint32_t max_frames) {
  auto* limiter = static_cast<LimiterClap*>(plugin->plugin_data);
  return limiter->Activate(sample_rate, min_frames, max_frames);
}

void ClapDeactivate(const clap_plugin_t* plugin) {
  auto* limiter = static_cast<LimiterClap*>(plugin->plugin_data);
  limiter->Deactivate();
}

bool ClapStartProcessing(const clap_plugin_t* plugin) {
  auto* limiter = static_cast<LimiterClap*>(plugin->plugin_data);
  return limiter->StartProcessing();
}

void ClapStopProcessing(const clap_plugin_t* plugin) {
  auto* limiter = static_cast<LimiterClap*>(plugin->plugin_data);
  limiter->StopProcessing();
}

void ClapReset(const clap_plugin_t* plugin) {
  auto* limiter = static_cast<LimiterClap*>(plugin->plugin_data);
  limiter->Reset();
}

clap_process_status ClapProcess(const clap_plugin_t* plugin,
                                 const clap_process_t* process) {
  auto* limiter = static_cast<LimiterClap*>(plugin->plugin_data);
  return limiter->Process(process);
}

const void* ClapGetExtension(const clap_plugin_t* plugin, const char* id) {
  auto* limiter = static_cast<LimiterClap*>(plugin->plugin_data);
  return limiter->GetExtension(id);
}

void ClapOnMainThread(const clap_plugin_t*) {}

// Parameters extension callbacks
uint32_t ClapParamsCount(const clap_plugin_t* plugin) {
  auto* limiter = static_cast<LimiterClap*>(plugin->plugin_data);
  return limiter->ParamsCount();
}

bool ClapParamsGetInfo(const clap_plugin_t* plugin, uint32_t param_index,
                       clap_param_info_t* param_info) {
  auto* limiter = static_cast<LimiterClap*>(plugin->plugin_data);
  return limiter->ParamsInfo(param_index, param_info);
}

bool ClapParamsGetValue(const clap_plugin_t* plugin, clap_id param_id,
                        double* value) {
  auto* limiter = static_cast<LimiterClap*>(plugin->plugin_data);
  return limiter->ParamsValue(param_id, value);
}

bool ClapParamsValueToText(const clap_plugin_t* plugin, clap_id param_id,
                           double value, char* display, uint32_t size) {
  auto* limiter = static_cast<LimiterClap*>(plugin->plugin_data);
  return limiter->ParamsValueToText(param_id, value, display, size);
}

bool ClapParamsTextToValue(const clap_plugin_t* plugin, clap_id param_id,
                           const char* display, double* value) {
  auto* limiter = static_cast<LimiterClap*>(plugin->plugin_data);
  return limiter->ParamsTextToValue(param_id, display, value);
}

void ClapParamsFlush(const clap_plugin_t* plugin,
                     const clap_input_events_t* in,
                     const clap_output_events_t* out) {
  auto* limiter = static_cast<LimiterClap*>(plugin->plugin_data);
  limiter->ParamsFlush(in, out);
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
  auto* limiter = static_cast<LimiterClap*>(plugin->plugin_data);
  return limiter->StateSave(stream);
}

bool ClapStateLoad(const clap_plugin_t* plugin, const clap_istream_t* stream) {
  auto* limiter = static_cast<LimiterClap*>(plugin->plugin_data);
  return limiter->StateLoad(stream);
}

static const clap_plugin_state_t kStateExtension = {
    ClapStateSave,
    ClapStateLoad,
};

// Audio ports extension callbacks
uint32_t ClapAudioPortsCount(const clap_plugin_t* plugin, bool is_input) {
  auto* limiter = static_cast<LimiterClap*>(plugin->plugin_data);
  return limiter->AudioPortsCount(is_input);
}

bool ClapAudioPortsGet(const clap_plugin_t* plugin, uint32_t index,
                       bool is_input, clap_audio_port_info_t* info) {
  auto* limiter = static_cast<LimiterClap*>(plugin->plugin_data);
  return limiter->AudioPortsGet(index, is_input, info);
}

static const clap_plugin_audio_ports_t kAudioPortsExtension = {
    ClapAudioPortsCount,
    ClapAudioPortsGet,
};

}  // namespace

LimiterClap::LimiterClap(const clap_host_t* host)
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
  param_values_[kParamIdThreshold].store(ThresholdToNormalized(-0.1));
  param_values_[kParamIdOutputLevel].store(OutputLevelToNormalized(-0.1));
}

bool LimiterClap::Init() noexcept {
  UpdateProcessorParams();
  return true;
}

bool LimiterClap::Activate(double sample_rate, uint32_t /*min_frames*/,
                            uint32_t /*max_frames*/) noexcept {
  sample_rate_ = sample_rate;
  processor_.Initialize(sample_rate);
  UpdateProcessorParams();  // Sync stored parameter values with processor
  return true;
}

void LimiterClap::Deactivate() noexcept {
  is_processing_ = false;
}

bool LimiterClap::StartProcessing() noexcept {
  is_processing_ = true;
  return true;
}

void LimiterClap::StopProcessing() noexcept {
  is_processing_ = false;
}

void LimiterClap::Reset() noexcept {
  processor_.Reset();
}

clap_process_status LimiterClap::Process(
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

  // Copy input to output
  std::memcpy(out_left, in_left, frame_count * sizeof(float));
  if (in_right && out_right) {
    std::memcpy(out_right, in_right, frame_count * sizeof(float));
  }

  // Process limiting
  if (out_right) {
    processor_.ProcessStereo(out_left, out_right, frame_count);
  } else {
    processor_.ProcessStereo(out_left, out_left, frame_count);
  }

  return CLAP_PROCESS_CONTINUE;
}

const void* LimiterClap::GetExtension(const char* id) noexcept {
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

uint32_t LimiterClap::ParamsCount() const noexcept {
  return kParamIdCount;
}

bool LimiterClap::ParamsInfo(uint32_t param_index,
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
      info->default_value = ThresholdToNormalized(-0.1);
      break;
    case kParamIdOutputLevel:
      std::snprintf(info->name, sizeof(info->name), "Output Level");
      std::snprintf(info->module, sizeof(info->module), "");
      info->min_value = 0.0;
      info->max_value = 1.0;
      info->default_value = OutputLevelToNormalized(-0.1);
      break;
    default:
      return false;
  }

  return true;
}

bool LimiterClap::ParamsValue(clap_id param_id, double* value) noexcept {
  if (param_id >= kParamIdCount) return false;
  *value = param_values_[param_id].load();
  return true;
}

bool LimiterClap::ParamsValueToText(clap_id param_id, double value,
                                    char* display,
                                    uint32_t size) noexcept {
  if (param_id >= kParamIdCount) return false;

  switch (param_id) {
    case kParamIdThreshold:
      std::snprintf(display, size, "%.2f dB", NormalizedToThreshold(value));
      break;
    case kParamIdOutputLevel:
      std::snprintf(display, size, "%.2f dB", NormalizedToOutputLevel(value));
      break;
    default:
      return false;
  }

  return true;
}

bool LimiterClap::ParamsTextToValue(clap_id param_id, const char* display,
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

  switch (param_id) {
    case kParamIdThreshold:
      *value = ThresholdToNormalized(std::clamp(parsed_value, kThresholdMin, kThresholdMax));
      break;
    case kParamIdOutputLevel:
      *value = OutputLevelToNormalized(std::clamp(parsed_value, kOutputLevelMin, kOutputLevelMax));
      break;
    default:
      return false;
  }

  return true;
}

void LimiterClap::ParamsFlush(const clap_input_events_t* in,
                              const clap_output_events_t* /*out*/) noexcept {
  ProcessParameterChanges(in);
}

bool LimiterClap::StateSave(const clap_ostream_t* stream) noexcept {
  double values[kParamIdCount];
  for (int i = 0; i < kParamIdCount; ++i) {
    values[i] = param_values_[i].load();
  }
  
  int64_t written = stream->write(stream, values, sizeof(values));
  return written == sizeof(values);
}

bool LimiterClap::StateLoad(const clap_istream_t* stream) noexcept {
  double values[kParamIdCount];
  int64_t read = stream->read(stream, values, sizeof(values));
  
  if (read != sizeof(values)) return false;

  for (int i = 0; i < kParamIdCount; ++i) {
    param_values_[i].store(values[i]);
  }

  UpdateProcessorParams();
  return true;
}

void LimiterClap::ProcessParameterChanges(
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

void LimiterClap::UpdateProcessorParams() noexcept {
  LimiterParams params;
  params.threshold_db = static_cast<float>(NormalizedToThreshold(param_values_[kParamIdThreshold].load()));
  params.output_level_db = static_cast<float>(NormalizedToOutputLevel(param_values_[kParamIdOutputLevel].load()));
  
  processor_.SetParams(params);
}

void LimiterClap::SetParamValue(clap_id param_id, double value) noexcept {
  if (param_id < kParamIdCount) {
    param_values_[param_id].store(value);
  }
}

uint32_t LimiterClap::AudioPortsCount(bool /*is_input*/) const noexcept {
  return 1;  // One stereo port for input and one for output
}

bool LimiterClap::AudioPortsGet(uint32_t index, bool is_input,
                                clap_audio_port_info_t* info) const noexcept {
  if (index > 0) return false;

  info->id = 0;
  std::snprintf(info->name, sizeof(info->name), is_input ? "Audio Input" : "Audio Output");
  info->channel_count = 2;
  info->flags = CLAP_AUDIO_PORT_IS_MAIN;
  info->port_type = CLAP_PORT_STEREO;
  info->in_place_pair = is_input ? 0 : 0;

  return true;
}

}  // namespace fast_limiter

// CLAP plugin factory
extern "C" {

static const clap_plugin_descriptor_t kDescriptor = {
    CLAP_VERSION,
    fast_limiter::kPluginId,
    fast_limiter::kPluginName,
    fast_limiter::kPluginVendor,
    fast_limiter::kPluginUrl,
    nullptr,
    nullptr,
    fast_limiter::kPluginVersion,
    fast_limiter::kPluginDescription,
    fast_limiter::kFeatures,
};

static const clap_plugin_t* ClapCreatePlugin(const clap_plugin_factory_t* /*factory*/,
                                              const clap_host_t* host,
                                              const char* plugin_id) {
  if (std::strcmp(plugin_id, fast_limiter::kPluginId) != 0) {
    return nullptr;
  }

  auto* plugin = new fast_limiter::LimiterClap(host);
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
