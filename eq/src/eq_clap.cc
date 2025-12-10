// Copyright 2025
// CLAP Plugin Wrapper Implementation for 4-Band EQ

#include "eq_clap.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace fast_eq {

namespace {

constexpr const char* kPluginId = "com.sesame.eq";
constexpr const char* kPluginName = "Sesame EQ";
constexpr const char* kPluginVendor = "Stinky Computing";
constexpr const char* kPluginUrl = "https://github.com/stinkydev/audio-plugins";
constexpr const char* kPluginVersion = "1.0.0";
constexpr const char* kPluginDescription = 
    "High-quality 4-band parametric EQ with multiple filter types";

constexpr const char* kFeatures[] = {
    CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
    CLAP_PLUGIN_FEATURE_EQUALIZER,
    CLAP_PLUGIN_FEATURE_STEREO,
    nullptr
};

// Parameter ranges
constexpr double kFreqMin = 20.0;
constexpr double kFreqMax = 20000.0;
constexpr double kGainMin = -24.0;
constexpr double kGainMax = 24.0;
constexpr double kQMin = 0.1;
constexpr double kQMax = 10.0;
constexpr double kOutputGainMin = -12.0;
constexpr double kOutputGainMax = 12.0;

// CLAP plugin callbacks
bool ClapInit(const clap_plugin_t* plugin) {
  auto* eq = static_cast<EqClap*>(plugin->plugin_data);
  return eq->Init();
}

void ClapDestroy(const clap_plugin_t* plugin) {
  auto* eq = static_cast<EqClap*>(plugin->plugin_data);
  delete eq;
}

bool ClapActivate(const clap_plugin_t* plugin, double sample_rate,
                  uint32_t min_frames, uint32_t max_frames) {
  auto* eq = static_cast<EqClap*>(plugin->plugin_data);
  return eq->Activate(sample_rate, min_frames, max_frames);
}

void ClapDeactivate(const clap_plugin_t* plugin) {
  auto* eq = static_cast<EqClap*>(plugin->plugin_data);
  eq->Deactivate();
}

bool ClapStartProcessing(const clap_plugin_t* plugin) {
  auto* eq = static_cast<EqClap*>(plugin->plugin_data);
  return eq->StartProcessing();
}

void ClapStopProcessing(const clap_plugin_t* plugin) {
  auto* eq = static_cast<EqClap*>(plugin->plugin_data);
  eq->StopProcessing();
}

void ClapReset(const clap_plugin_t* plugin) {
  auto* eq = static_cast<EqClap*>(plugin->plugin_data);
  eq->Reset();
}

clap_process_status ClapProcess(const clap_plugin_t* plugin,
                                 const clap_process_t* process) {
  auto* eq = static_cast<EqClap*>(plugin->plugin_data);
  return eq->Process(process);
}

const void* ClapGetExtension(const clap_plugin_t* plugin, const char* id) {
  auto* eq = static_cast<EqClap*>(plugin->plugin_data);
  return eq->GetExtension(id);
}

void ClapOnMainThread(const clap_plugin_t*) {}

// Parameters extension callbacks
uint32_t ClapParamsCount(const clap_plugin_t* plugin) {
  auto* eq = static_cast<EqClap*>(plugin->plugin_data);
  return eq->ParamsCount();
}

bool ClapParamsGetInfo(const clap_plugin_t* plugin, uint32_t param_index,
                       clap_param_info_t* param_info) {
  auto* eq = static_cast<EqClap*>(plugin->plugin_data);
  return eq->ParamsInfo(param_index, param_info);
}

bool ClapParamsGetValue(const clap_plugin_t* plugin, clap_id param_id,
                        double* value) {
  auto* eq = static_cast<EqClap*>(plugin->plugin_data);
  return eq->ParamsValue(param_id, value);
}

bool ClapParamsValueToText(const clap_plugin_t* plugin, clap_id param_id,
                           double value, char* display, uint32_t size) {
  auto* eq = static_cast<EqClap*>(plugin->plugin_data);
  return eq->ParamsValueToText(param_id, value, display, size);
}

bool ClapParamsTextToValue(const clap_plugin_t* plugin, clap_id param_id,
                           const char* display, double* value) {
  auto* eq = static_cast<EqClap*>(plugin->plugin_data);
  return eq->ParamsTextToValue(param_id, display, value);
}

void ClapParamsFlush(const clap_plugin_t* plugin,
                     const clap_input_events_t* in,
                     const clap_output_events_t* out) {
  auto* eq = static_cast<EqClap*>(plugin->plugin_data);
  eq->ParamsFlush(in, out);
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
  auto* eq = static_cast<EqClap*>(plugin->plugin_data);
  return eq->StateSave(stream);
}

bool ClapStateLoad(const clap_plugin_t* plugin, const clap_istream_t* stream) {
  auto* eq = static_cast<EqClap*>(plugin->plugin_data);
  return eq->StateLoad(stream);
}

static const clap_plugin_state_t kStateExtension = {
    ClapStateSave,
    ClapStateLoad,
};

// Audio ports extension callbacks
uint32_t ClapAudioPortsCount(const clap_plugin_t* plugin, bool is_input) {
  auto* eq = static_cast<EqClap*>(plugin->plugin_data);
  return eq->AudioPortsCount(is_input);
}

bool ClapAudioPortsGet(const clap_plugin_t* plugin, uint32_t index,
                       bool is_input, clap_audio_port_info_t* info) {
  auto* eq = static_cast<EqClap*>(plugin->plugin_data);
  return eq->AudioPortsGet(index, is_input, info);
}

static const clap_plugin_audio_ports_t kAudioPortsExtension = {
    ClapAudioPortsCount,
    ClapAudioPortsGet,
};

}  // namespace

EqClap::EqClap(const clap_host_t* host)
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
  // Band 1 (Low Shelf)
  param_values_[kParamIdBand1Type].store(static_cast<double>(FilterType::kLowShelf));
  param_values_[kParamIdBand1Freq].store(100.0);
  param_values_[kParamIdBand1Gain].store(0.0);
  param_values_[kParamIdBand1Q].store(0.707);
  param_values_[kParamIdBand1Enable].store(1.0);
  
  // Band 2 (Peak)
  param_values_[kParamIdBand2Type].store(static_cast<double>(FilterType::kBell));
  param_values_[kParamIdBand2Freq].store(500.0);
  param_values_[kParamIdBand2Gain].store(0.0);
  param_values_[kParamIdBand2Q].store(1.0);
  param_values_[kParamIdBand2Enable].store(1.0);
  
  // Band 3 (Peak)
  param_values_[kParamIdBand3Type].store(static_cast<double>(FilterType::kBell));
  param_values_[kParamIdBand3Freq].store(2000.0);
  param_values_[kParamIdBand3Gain].store(0.0);
  param_values_[kParamIdBand3Q].store(1.0);
  param_values_[kParamIdBand3Enable].store(1.0);
  
  // Band 4 (High Shelf)
  param_values_[kParamIdBand4Type].store(static_cast<double>(FilterType::kHighShelf));
  param_values_[kParamIdBand4Freq].store(8000.0);
  param_values_[kParamIdBand4Gain].store(0.0);
  param_values_[kParamIdBand4Q].store(0.707);
  param_values_[kParamIdBand4Enable].store(1.0);
  
  // Global
  param_values_[kParamIdOutputGain].store(0.0);
  param_values_[kParamIdBypass].store(0.0);
}

bool EqClap::Init() noexcept {
  UpdateProcessorParams();
  return true;
}

bool EqClap::Activate(double sample_rate, uint32_t /*min_frames*/,
                      uint32_t /*max_frames*/) noexcept {
  sample_rate_ = sample_rate;
  processor_.Initialize(sample_rate);
  UpdateProcessorParams();
  return true;
}

void EqClap::Deactivate() noexcept {
  is_processing_ = false;
}

bool EqClap::StartProcessing() noexcept {
  is_processing_ = true;
  return true;
}

void EqClap::StopProcessing() noexcept {
  is_processing_ = false;
}

void EqClap::Reset() noexcept {
  processor_.Reset();
}

clap_process_status EqClap::Process(const clap_process_t* process) noexcept {
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

  // Process EQ
  if (out_right) {
    processor_.ProcessStereo(out_left, out_right, frame_count);
  } else {
    processor_.ProcessStereo(out_left, out_left, frame_count);
  }

  return CLAP_PROCESS_CONTINUE;
}

const void* EqClap::GetExtension(const char* id) noexcept {
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

uint32_t EqClap::ParamsCount() const noexcept {
  return kParamIdCount;
}

bool EqClap::ParamsInfo(uint32_t param_index,
                        clap_param_info_t* info) const noexcept {
  if (param_index >= kParamIdCount) return false;

  info->id = param_index;
  info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE;
  info->cookie = nullptr;

  // Helper to get band number (0-3) from param_index
  auto get_band = [](uint32_t idx) { return (idx / 5); };
  auto get_param_type = [](uint32_t idx) { return idx % 5; };

  if (param_index < kParamIdOutputGain) {
    const int band = get_band(param_index) + 1;
    const int param_type = get_param_type(param_index);
    
    switch (param_type) {
      case 0:  // Type
        std::snprintf(info->name, sizeof(info->name), "Band %d Type", band);
        std::snprintf(info->module, sizeof(info->module), "Band %d", band);
        info->min_value = 0.0;
        info->max_value = 4.0;  // 5 filter types: HighPass, Peak, LowShelf, HighShelf, LowPass
        info->default_value = (band == 1 || band == 4) ? 
            static_cast<double>(band == 1 ? FilterType::kLowShelf : FilterType::kHighShelf) :
            static_cast<double>(FilterType::kBell);
        info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_STEPPED;
        break;
      case 1:  // Frequency
        std::snprintf(info->name, sizeof(info->name), "Band %d Frequency", band);
        std::snprintf(info->module, sizeof(info->module), "Band %d", band);
        info->min_value = kFreqMin;
        info->max_value = kFreqMax;
        info->default_value = (band == 1) ? 100.0 : (band == 2) ? 500.0 : 
                              (band == 3) ? 2000.0 : 8000.0;
        break;
      case 2:  // Gain
        std::snprintf(info->name, sizeof(info->name), "Band %d Gain", band);
        std::snprintf(info->module, sizeof(info->module), "Band %d", band);
        info->min_value = kGainMin;
        info->max_value = kGainMax;
        info->default_value = 0.0;
        break;
      case 3:  // Q
        std::snprintf(info->name, sizeof(info->name), "Band %d Q", band);
        std::snprintf(info->module, sizeof(info->module), "Band %d", band);
        info->min_value = kQMin;
        info->max_value = kQMax;
        info->default_value = (band == 2 || band == 3) ? 1.0 : 0.707;
        break;
      case 4:  // Enable
        std::snprintf(info->name, sizeof(info->name), "Band %d Enable", band);
        std::snprintf(info->module, sizeof(info->module), "Band %d", band);
        info->min_value = 0.0;
        info->max_value = 1.0;
        info->default_value = 1.0;
        info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_STEPPED;
        break;
    }
  } else if (param_index == kParamIdOutputGain) {
    std::snprintf(info->name, sizeof(info->name), "Output Gain");
    std::snprintf(info->module, sizeof(info->module), "");
    info->min_value = kOutputGainMin;
    info->max_value = kOutputGainMax;
    info->default_value = 0.0;
  } else if (param_index == kParamIdBypass) {
    std::snprintf(info->name, sizeof(info->name), "Bypass");
    std::snprintf(info->module, sizeof(info->module), "");
    info->min_value = 0.0;
    info->max_value = 1.0;
    info->default_value = 0.0;
    info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_STEPPED;
  } else {
    return false;
  }

  return true;
}

bool EqClap::ParamsValue(clap_id param_id, double* value) noexcept {
  if (param_id >= kParamIdCount) return false;
  *value = param_values_[param_id].load();
  return true;
}

bool EqClap::ParamsValueToText(clap_id param_id, double value,
                               char* display, uint32_t size) noexcept {
  if (param_id >= kParamIdCount) return false;

  auto get_param_type = [](clap_id idx) { return idx % 5; };

  if (param_id < kParamIdOutputGain) {
    const int param_type = get_param_type(param_id);
    
    switch (param_type) {
      case 0:  // Type
        {
          const char* type_names[] = {
            "Low Cut", "Bell", "Low Shelf", "High Shelf", "High Cut"
          };
          const int type_idx = static_cast<int>(value);
          if (type_idx >= 0 && type_idx < 5) {
            std::snprintf(display, size, "%s", type_names[type_idx]);
          } else {
            return false;
          }
        }
        break;
      case 1:  // Frequency
        if (value >= 1000.0) {
          std::snprintf(display, size, "%.2f kHz", value / 1000.0);
        } else {
          std::snprintf(display, size, "%.1f Hz", value);
        }
        break;
      case 2:  // Gain
        std::snprintf(display, size, "%.1f dB", value);
        break;
      case 3:  // Q
        std::snprintf(display, size, "%.2f", value);
        break;
      case 4:  // Enable
        std::snprintf(display, size, "%s", value > 0.5 ? "On" : "Off");
        break;
    }
  } else if (param_id == kParamIdOutputGain) {
    std::snprintf(display, size, "%.1f dB", value);
  } else if (param_id == kParamIdBypass) {
    std::snprintf(display, size, "%s", value > 0.5 ? "On" : "Off");
  } else {
    return false;
  }

  return true;
}

bool EqClap::ParamsTextToValue(clap_id param_id, const char* display,
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

void EqClap::ParamsFlush(const clap_input_events_t* in,
                         const clap_output_events_t* /*out*/) noexcept {
  ProcessParameterChanges(in);
}

bool EqClap::StateSave(const clap_ostream_t* stream) noexcept {
  double values[kParamIdCount];
  for (int i = 0; i < kParamIdCount; ++i) {
    values[i] = param_values_[i].load();
  }
  
  int64_t written = stream->write(stream, values, sizeof(values));
  return written == sizeof(values);
}

bool EqClap::StateLoad(const clap_istream_t* stream) noexcept {
  double values[kParamIdCount];
  int64_t read = stream->read(stream, values, sizeof(values));
  
  if (read != sizeof(values)) return false;

  for (int i = 0; i < kParamIdCount; ++i) {
    param_values_[i].store(values[i]);
  }

  UpdateProcessorParams();
  return true;
}

void EqClap::ProcessParameterChanges(const clap_input_events_t* events) noexcept {
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

void EqClap::UpdateProcessorParams() noexcept {
  EqParams params;
  
  // Update each band
  for (int i = 0; i < 4; ++i) {
    const int base_idx = i * 5;
    params.bands[i].type = static_cast<FilterType>(
        static_cast<int>(param_values_[base_idx + 0].load()));
    params.bands[i].frequency_hz = static_cast<float>(param_values_[base_idx + 1].load());
    params.bands[i].gain_db = static_cast<float>(param_values_[base_idx + 2].load());
    params.bands[i].q = static_cast<float>(param_values_[base_idx + 3].load());
    params.bands[i].enabled = param_values_[base_idx + 4].load() > 0.5;
  }
  
  params.output_gain_db = static_cast<float>(param_values_[kParamIdOutputGain].load());
  params.bypass = param_values_[kParamIdBypass].load() > 0.5;
  
  processor_.SetParams(params);
}

void EqClap::SetParamValue(clap_id param_id, double value) noexcept {
  if (param_id < kParamIdCount) {
    param_values_[param_id].store(value);
  }
}

uint32_t EqClap::AudioPortsCount(bool /*is_input*/) const noexcept {
  return 1;
}

bool EqClap::AudioPortsGet(uint32_t index, bool is_input,
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

}  // namespace fast_eq

// CLAP plugin factory
extern "C" {

static const clap_plugin_descriptor_t kDescriptor = {
    CLAP_VERSION,
    fast_eq::kPluginId,
    fast_eq::kPluginName,
    fast_eq::kPluginVendor,
    fast_eq::kPluginUrl,
    nullptr,
    nullptr,
    fast_eq::kPluginVersion,
    fast_eq::kPluginDescription,
    fast_eq::kFeatures,
};

static const clap_plugin_t* ClapCreatePlugin(const clap_plugin_factory_t* /*factory*/,
                                              const clap_host_t* host,
                                              const char* plugin_id) {
  if (std::strcmp(plugin_id, fast_eq::kPluginId) != 0) {
    return nullptr;
  }

  auto* plugin = new fast_eq::EqClap(host);
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
