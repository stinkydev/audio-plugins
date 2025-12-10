// Copyright 2025
// CLAP Plugin Wrapper Implementation

#include "compressor_clap.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace fast_compressor {

namespace {

constexpr const char* kPluginId = "com.sesame.compressor";
constexpr const char* kPluginName = "Sesame Compressor";
constexpr const char* kPluginVendor = "Stinky Computing AB";
constexpr const char* kPluginUrl = "https://github.com/fastcompressor";
constexpr const char* kPluginVersion = "1.0.0";
constexpr const char* kPluginDescription = 
    "High-performance audio compressor with SIMD optimization";

constexpr const char* kFeatures[] = {
    CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
    CLAP_PLUGIN_FEATURE_COMPRESSOR,
    CLAP_PLUGIN_FEATURE_STEREO,
    nullptr
};

// Parameter ranges
constexpr double kThresholdMin = -60.0;
constexpr double kThresholdMax = 0.0;
constexpr double kRatioMin = 1.0;
constexpr double kRatioMax = 20.0;
constexpr double kAttackMin = 0.1;
constexpr double kAttackMax = 100.0;
constexpr double kReleaseMin = 10.0;
constexpr double kReleaseMax = 1000.0;
constexpr double kKneeMin = 0.0;
constexpr double kKneeMax = 12.0;
constexpr double kMakeupMin = -12.0;
constexpr double kMakeupMax = 24.0;

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

  // Initialize parameters to defaults
  param_values_[kParamIdThreshold].store(-20.0);
  param_values_[kParamIdRatio].store(4.0);
  param_values_[kParamIdAttack].store(5.0);
  param_values_[kParamIdRelease].store(50.0);
  param_values_[kParamIdKnee].store(0.0);
  param_values_[kParamIdMakeupGain].store(0.0);
}

bool CompressorClap::Init() noexcept {
  UpdateProcessorParams();
  return true;
}

bool CompressorClap::Activate(double sample_rate, uint32_t /*min_frames*/,
                               uint32_t /*max_frames*/) noexcept {
  sample_rate_ = sample_rate;
  processor_.Initialize(sample_rate);
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

  // Get stereo channels
  float* in_left = process->audio_inputs[0].data32[0];
  float* in_right = process->audio_inputs[0].data32[1];
  float* out_left = process->audio_outputs[0].data32[0];
  float* out_right = process->audio_outputs[0].data32[1];

  // Copy input to output
  std::memcpy(out_left, in_left, frame_count * sizeof(float));
  std::memcpy(out_right, in_right, frame_count * sizeof(float));

  // Process compression
  processor_.ProcessStereo(out_left, out_right, frame_count);

  return CLAP_PROCESS_CONTINUE;
}

const void* CompressorClap::GetExtension(const char* id) noexcept {
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
      info->min_value = kThresholdMin;
      info->max_value = kThresholdMax;
      info->default_value = -20.0;
      break;
    case kParamIdRatio:
      std::snprintf(info->name, sizeof(info->name), "Ratio");
      std::snprintf(info->module, sizeof(info->module), "");
      info->min_value = kRatioMin;
      info->max_value = kRatioMax;
      info->default_value = 4.0;
      break;
    case kParamIdAttack:
      std::snprintf(info->name, sizeof(info->name), "Attack");
      std::snprintf(info->module, sizeof(info->module), "");
      info->min_value = kAttackMin;
      info->max_value = kAttackMax;
      info->default_value = 5.0;
      break;
    case kParamIdRelease:
      std::snprintf(info->name, sizeof(info->name), "Release");
      std::snprintf(info->module, sizeof(info->module), "");
      info->min_value = kReleaseMin;
      info->max_value = kReleaseMax;
      info->default_value = 50.0;
      break;
    case kParamIdKnee:
      std::snprintf(info->name, sizeof(info->name), "Knee");
      std::snprintf(info->module, sizeof(info->module), "");
      info->min_value = kKneeMin;
      info->max_value = kKneeMax;
      info->default_value = 0.0;
      break;
    case kParamIdMakeupGain:
      std::snprintf(info->name, sizeof(info->name), "Makeup Gain");
      std::snprintf(info->module, sizeof(info->module), "");
      info->min_value = kMakeupMin;
      info->max_value = kMakeupMax;
      info->default_value = 0.0;
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
    case kParamIdKnee:
    case kParamIdMakeupGain:
      std::snprintf(display, size, "%.1f dB", value);
      break;
    case kParamIdRatio:
      std::snprintf(display, size, "%.1f:1", value);
      break;
    case kParamIdAttack:
    case kParamIdRelease:
      std::snprintf(display, size, "%.1f ms", value);
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

  clap_param_info_t info;
  if (!ParamsInfo(param_id, &info)) return false;

  *value = std::clamp(parsed_value, info.min_value, info.max_value);
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
  params.threshold_db = static_cast<float>(param_values_[kParamIdThreshold].load());
  params.ratio = static_cast<float>(param_values_[kParamIdRatio].load());
  params.attack_ms = static_cast<float>(param_values_[kParamIdAttack].load());
  params.release_ms = static_cast<float>(param_values_[kParamIdRelease].load());
  params.knee_db = static_cast<float>(param_values_[kParamIdKnee].load());
  params.makeup_gain_db = static_cast<float>(param_values_[kParamIdMakeupGain].load());
  
  processor_.SetParams(params);
}

void CompressorClap::SetParamValue(clap_id param_id, double value) noexcept {
  if (param_id < kParamIdCount) {
    param_values_[param_id].store(value);
  }
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
