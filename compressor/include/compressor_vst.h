// Copyright 2025
// This file is deprecated - use compressor_clap.h instead

#ifndef COMPRESSOR_VST_H_
#define COMPRESSOR_VST_H_

#warning "VST2 support has been removed. Use CLAP format instead."

namespace fast_compressor {

// VST parameter indices
enum CompressorParam {
  kParamThreshold = 0,
  kParamRatio,
  kParamAttack,
  kParamRelease,
  kParamKnee,
  kParamMakeupGain,
  kNumParams
};

class CompressorVst : public AudioEffectX {
 public:
  CompressorVst(audioMasterCallback audio_master);
  ~CompressorVst() override = default;

  // Processing
  void processReplacing(float** inputs, float** outputs,
                       VstInt32 sample_frames) override;
  void processDoubleReplacing(double** inputs, double** outputs,
                             VstInt32 sample_frames) override;

  // Parameters
  void setParameter(VstInt32 index, float value) override;
  float getParameter(VstInt32 index) override;
  void getParameterLabel(VstInt32 index, char* label) override;
  void getParameterDisplay(VstInt32 index, char* text) override;
  void getParameterName(VstInt32 index, char* text) override;

  // Plugin properties
  bool getEffectName(char* name) override;
  bool getVendorString(char* text) override;
  bool getProductString(char* text) override;
  VstInt32 getVendorVersion() override;
  VstInt32 canDo(char* text) override;

  // State
  void suspend() override;
  void resume() override;
  void setSampleRate(float sample_rate) override;

 private:
  void UpdateProcessorParams();
  float ParamToDb(float param, float min_db, float max_db) const;
  float DbToParam(float db, float min_db, float max_db) const;

  CompressorProcessor processor_;
  float parameters_[kNumParams];
};

}  // namespace fast_compressor

#endif  // COMPRESSOR_VST_H_
