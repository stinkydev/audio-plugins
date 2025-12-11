// Copyright 2025
// CLAP Plugin Wrapper Implementation for Delay

#include "delay_clap.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace stinky_delay {

namespace {

constexpr const char* kPluginId = "com.stinky.delay";
constexpr const char* kPluginName = "Delay";
constexpr const char* kPluginVendor = "Stinky";
constexpr const char* kPluginUrl = "https://github.com/stinkydev/audio-plugins";
constexpr const char* kPluginVersion = "1.0.0";
constexpr const char* kPluginDescription = 
    "Simple stereo delay effect";

constexpr const char* kFeatures[] = {
    CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
    CLAP_PLUGIN_FEATURE_DELAY,
    CLAP_PLUGIN_FEATURE_STEREO,
    nullptr
};

// Conversion functions from normalized [0,1] to actual values
inline double NormalizedToDelayTime(double norm) {
  return kDelayTimeMin + norm * (kDelayTimeMax - kDelayTimeMin);
}

inline double DelayTimeToNormalized(double ms) {
  return (ms - kDelayTimeMin) / (kDelayTimeMax - kDelayTimeMin);
}

inline double NormalizedToMix(double norm) {
  return kMixMin + norm * (kMixMax - kMixMin);
}

inline double MixToNormalized(double value) {
  return (value - kMixMin) / (kMixMax - kMixMin);
}

// CLAP plugin callbacks
bool ClapInit(const clap_plugin_t* plugin) {
  auto* delay = static_cast<DelayClap*>(plugin->plugin_data);
  return delay->Init();
}

void ClapDestroy(const clap_plugin_t* plugin) {
  auto* delay = static_cast<DelayClap*>(plugin->plugin_data);
  delete delay;
}

bool ClapActivate(const clap_plugin_t* plugin, double sample_rate,
                  uint32_t min_frames, uint32_t max_frames) {
  auto* delay = static_cast<DelayClap*>(plugin->plugin_data);
  return delay->Activate(sample_rate, min_frames, max_frames);
}

void ClapDeactivate(const clap_plugin_t* plugin) {
  auto* delay = static_cast<DelayClap*>(plugin->plugin_data);
  delay->Deactivate();
}

bool ClapStartProcessing(const clap_plugin_t* plugin) {
  auto* delay = static_cast<DelayClap*>(plugin->plugin_data);
  return delay->StartProcessing();
}

void ClapStopProcessing(const clap_plugin_t* plugin) {
  auto* delay = static_cast<DelayClap*>(plugin->plugin_data);
  delay->StopProcessing();
}

void ClapReset(const clap_plugin_t* plugin) {
  auto* delay = static_cast<DelayClap*>(plugin->plugin_data);
  delay->Reset();
}

clap_process_status ClapProcess(const clap_plugin_t* plugin,
                                 const clap_process_t* process) {
  auto* delay = static_cast<DelayClap*>(plugin->plugin_data);
  return delay->Process(process);
}

const void* ClapGetExtension(const clap_plugin_t* plugin, const char* id) {
  auto* delay = static_cast<DelayClap*>(plugin->plugin_data);
  return delay->GetExtension(id);
}

void ClapOnMainThread(const clap_plugin_t*) {}

// Parameters extension callbacks
uint32_t ClapParamsCount(const clap_plugin_t* plugin) {
  auto* delay = static_cast<DelayClap*>(plugin->plugin_data);
  return delay->ParamsCount();
}

bool ClapParamsGetInfo(const clap_plugin_t* plugin, uint32_t param_index,
                       clap_param_info_t* param_info) {
  auto* delay = static_cast<DelayClap*>(plugin->plugin_data);
  return delay->ParamsInfo(param_index, param_info);
}

bool ClapParamsGetValue(const clap_plugin_t* plugin, clap_id param_id,
                        double* value) {
  auto* delay = static_cast<DelayClap*>(plugin->plugin_data);
  return delay->ParamsValue(param_id, value);
}

bool ClapParamsValueToText(const clap_plugin_t* plugin, clap_id param_id,
                           double value, char* display, uint32_t size) {
  auto* delay = static_cast<DelayClap*>(plugin->plugin_data);
  return delay->ParamsValueToText(param_id, value, display, size);
}

bool ClapParamsTextToValue(const clap_plugin_t* plugin, clap_id param_id,
                           const char* display, double* value) {
  auto* delay = static_cast<DelayClap*>(plugin->plugin_data);
  return delay->ParamsTextToValue(param_id, display, value);
}

void ClapParamsFlush(const clap_plugin_t* plugin,
                     const clap_input_events_t* in,
                     const clap_output_events_t* out) {
  auto* delay = static_cast<DelayClap*>(plugin->plugin_data);
  delay->ParamsFlush(in, out);
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
  auto* delay = static_cast<DelayClap*>(plugin->plugin_data);
  return delay->StateSave(stream);
}

bool ClapStateLoad(const clap_plugin_t* plugin, const clap_istream_t* stream) {
  auto* delay = static_cast<DelayClap*>(plugin->plugin_data);
  return delay->StateLoad(stream);
}

static const clap_plugin_state_t kStateExtension = {
    ClapStateSave,
    ClapStateLoad,
};

// Audio ports extension callbacks
uint32_t ClapAudioPortsCount(const clap_plugin_t* plugin, bool is_input) {
  auto* delay = static_cast<DelayClap*>(plugin->plugin_data);
  return delay->AudioPortsCount(is_input);
}

bool ClapAudioPortsGet(const clap_plugin_t* plugin, uint32_t index,
                       bool is_input, clap_audio_port_info_t* info) {
  auto* delay = static_cast<DelayClap*>(plugin->plugin_data);
  return delay->AudioPortsGet(index, is_input, info);
}

static const clap_plugin_audio_ports_t kAudioPortsExtension = {
    ClapAudioPortsCount,
    ClapAudioPortsGet,
};

}  // namespace

DelayClap::DelayClap(const clap_host_t* host)
    : host_(host),
      sample_rate_(44100.0),
      is_processing_(false) {
  plugin_.desc = nullptr;
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
  param_values_[kParamIdDelayTime].store(DelayTimeToNormalized(0.0));
  param_values_[kParamIdMix].store(MixToNormalized(1.0));
}

bool DelayClap::Init() noexcept {
  UpdateProcessorParams();
  return true;
}

bool DelayClap::Activate(double sample_rate, uint32_t /*min_frames*/,
                          uint32_t /*max_frames*/) noexcept {
  sample_rate_ = sample_rate;
  processor_.Initialize(sample_rate);
  UpdateProcessorParams();
  return true;
}

void DelayClap::Deactivate() noexcept {
  is_processing_ = false;
}

bool DelayClap::StartProcessing() noexcept {
  is_processing_ = true;
  return true;
}

void DelayClap::StopProcessing() noexcept {
  is_processing_ = false;
}

void DelayClap::Reset() noexcept {
  processor_.Reset();
}

clap_process_status DelayClap::Process(const clap_process_t* process) noexcept {
  if (process->in_events) {
    ProcessParameterChanges(process->in_events);
  }

  const uint32_t frame_count = process->frames_count;
  const uint32_t input_count = process->audio_inputs_count;
  const uint32_t output_count = process->audio_outputs_count;

  if (input_count == 0 || output_count == 0 || frame_count == 0) {
    return CLAP_PROCESS_SLEEP;
  }

  const uint32_t in_channels = process->audio_inputs[0].channel_count;
  const uint32_t out_channels = process->audio_outputs[0].channel_count;
  
  float* in_left = process->audio_inputs[0].data32[0];
  float* in_right = (in_channels > 1) ? process->audio_inputs[0].data32[1] : nullptr;
  float* out_left = process->audio_outputs[0].data32[0];
  float* out_right = (out_channels > 1) ? process->audio_outputs[0].data32[1] : nullptr;

  std::memcpy(out_left, in_left, frame_count * sizeof(float));
  if (in_right && out_right) {
    std::memcpy(out_right, in_right, frame_count * sizeof(float));
  }

  if (out_right) {
    processor_.ProcessStereo(out_left, out_right, frame_count);
  } else {
    processor_.ProcessStereo(out_left, out_left, frame_count);
  }

  return CLAP_PROCESS_CONTINUE;
}

const void* DelayClap::GetExtension(const char* id) noexcept {
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

uint32_t DelayClap::ParamsCount() const noexcept {
  return kParamIdCount;
}

bool DelayClap::ParamsInfo(uint32_t param_index,
                           clap_param_info_t* info) const noexcept {
  if (param_index >= kParamIdCount) return false;

  info->id = param_index;
  info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE;
  info->cookie = nullptr;

  switch (param_index) {
    case kParamIdDelayTime:
      std::snprintf(info->name, sizeof(info->name), "Delay Time");
      std::snprintf(info->module, sizeof(info->module), "");
      info->min_value = 0.0;
      info->max_value = 1.0;
      info->default_value = DelayTimeToNormalized(0.0);
      break;
    case kParamIdMix:
      std::snprintf(info->name, sizeof(info->name), "Mix");
      std::snprintf(info->module, sizeof(info->module), "");
      info->min_value = 0.0;
      info->max_value = 1.0;
      info->default_value = MixToNormalized(1.0);
      break;
    default:
      return false;
  }

  return true;
}

bool DelayClap::ParamsValue(clap_id param_id, double* value) noexcept {
  if (param_id >= kParamIdCount) return false;
  *value = param_values_[param_id].load();
  return true;
}

bool DelayClap::ParamsValueToText(clap_id param_id, double value,
                                   char* display, uint32_t size) noexcept {
  if (param_id >= kParamIdCount) return false;

  switch (param_id) {
    case kParamIdDelayTime:
      std::snprintf(display, size, "%.1f ms", NormalizedToDelayTime(value));
      break;
    case kParamIdMix:
      std::snprintf(display, size, "%.1f%%", NormalizedToMix(value) * 100.0);
      break;
    default:
      return false;
  }

  return true;
}

bool DelayClap::ParamsTextToValue(clap_id param_id, const char* display,
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
    case kParamIdDelayTime:
      *value = DelayTimeToNormalized(std::clamp(parsed_value, kDelayTimeMin, kDelayTimeMax));
      break;
    case kParamIdMix:
      *value = MixToNormalized(std::clamp(parsed_value, kMixMin, kMixMax));
      break;
    default:
      return false;
  }

  return true;
}

void DelayClap::ParamsFlush(const clap_input_events_t* in,
                             const clap_output_events_t* /*out*/) noexcept {
  ProcessParameterChanges(in);
}

bool DelayClap::StateSave(const clap_ostream_t* stream) noexcept {
  double values[kParamIdCount];
  for (int i = 0; i < kParamIdCount; ++i) {
    values[i] = param_values_[i].load();
  }
  
  int64_t written = stream->write(stream, values, sizeof(values));
  return written == sizeof(values);
}

bool DelayClap::StateLoad(const clap_istream_t* stream) noexcept {
  double values[kParamIdCount];
  int64_t read = stream->read(stream, values, sizeof(values));
  
  if (read != sizeof(values)) return false;

  for (int i = 0; i < kParamIdCount; ++i) {
    param_values_[i].store(values[i]);
  }

  UpdateProcessorParams();
  return true;
}

void DelayClap::ProcessParameterChanges(const clap_input_events_t* events) noexcept {
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

void DelayClap::UpdateProcessorParams() noexcept {
  DelayParams params;
  params.delay_time_ms = static_cast<float>(NormalizedToDelayTime(param_values_[kParamIdDelayTime].load()));
  params.mix = static_cast<float>(NormalizedToMix(param_values_[kParamIdMix].load()));
  
  processor_.SetParams(params);
}

void DelayClap::SetParamValue(clap_id param_id, double value) noexcept {
  if (param_id < kParamIdCount) {
    param_values_[param_id].store(value);
  }
}

uint32_t DelayClap::AudioPortsCount(bool /*is_input*/) const noexcept {
  return 1;
}

bool DelayClap::AudioPortsGet(uint32_t index, bool is_input,
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

}  // namespace stinky_delay

// CLAP plugin factory
extern "C" {

static const clap_plugin_descriptor_t kDescriptor = {
    CLAP_VERSION,
    stinky_delay::kPluginId,
    stinky_delay::kPluginName,
    stinky_delay::kPluginVendor,
    stinky_delay::kPluginUrl,
    nullptr,
    nullptr,
    stinky_delay::kPluginVersion,
    stinky_delay::kPluginDescription,
    stinky_delay::kFeatures,
};

static const clap_plugin_t* ClapCreatePlugin(const clap_plugin_factory_t* /*factory*/,
                                              const clap_host_t* host,
                                              const char* plugin_id) {
  if (std::strcmp(plugin_id, stinky_delay::kPluginId) != 0) {
    return nullptr;
  }

  auto* plugin = new stinky_delay::DelayClap(host);
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
