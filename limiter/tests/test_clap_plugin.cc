// Copyright 2025
// End-to-end tests for CLAP Limiter plugin

#include "limiter_clap.h"

#include <gtest/gtest.h>

#include <cstring>
#include <memory>
#include <vector>

namespace fast_limiter {
namespace {

// Mock CLAP host
class MockClapHost {
 public:
  MockClapHost() {
    host_.clap_version = CLAP_VERSION;
    host_.host_data = this;
    host_.name = "Test Host";
    host_.vendor = "Test Vendor";
    host_.url = "https://test.com";
    host_.version = "1.0.0";
    host_.get_extension = [](const clap_host_t*, const char*) -> const void* {
      return nullptr;
    };
    host_.request_restart = [](const clap_host_t*) {};
    host_.request_process = [](const clap_host_t*) {};
    host_.request_callback = [](const clap_host_t*) {};
  }

  const clap_host_t* Host() { return &host_; }

 private:
  clap_host_t host_;
};

class ClapPluginTest : public ::testing::Test {
 protected:
  void SetUp() override {
    host_ = std::make_unique<MockClapHost>();
    plugin_ = std::make_unique<LimiterClap>(host_->Host());
    ASSERT_TRUE(plugin_->Init());
  }

  void TearDown() override {
    plugin_.reset();
    host_.reset();
  }

  std::unique_ptr<MockClapHost> host_;
  std::unique_ptr<LimiterClap> plugin_;
};

TEST_F(ClapPluginTest, InitSucceeds) {
  EXPECT_NE(plugin_, nullptr);
}

TEST_F(ClapPluginTest, ActivateSucceeds) {
  EXPECT_TRUE(plugin_->Activate(44100.0, 64, 512));
}

TEST_F(ClapPluginTest, StartProcessingSucceeds) {
  plugin_->Activate(44100.0, 64, 512);
  EXPECT_TRUE(plugin_->StartProcessing());
}

TEST_F(ClapPluginTest, DeactivateSucceeds) {
  plugin_->Activate(44100.0, 64, 512);
  plugin_->Deactivate();
  SUCCEED();
}

TEST_F(ClapPluginTest, ResetSucceeds) {
  plugin_->Activate(44100.0, 64, 512);
  plugin_->Reset();
  SUCCEED();
}

TEST_F(ClapPluginTest, ParamsCountReturnsCorrectValue) {
  EXPECT_EQ(plugin_->ParamsCount(), kParamIdCount);
}

TEST_F(ClapPluginTest, ParamsInfoReturnsValidInfo) {
  clap_param_info_t info;
  
  for (uint32_t i = 0; i < plugin_->ParamsCount(); ++i) {
    EXPECT_TRUE(plugin_->ParamsInfo(i, &info));
    EXPECT_EQ(info.id, i);
    EXPECT_NE(info.name[0], '\0');
    EXPECT_LE(info.min_value, info.max_value);
    EXPECT_GE(info.default_value, info.min_value);
    EXPECT_LE(info.default_value, info.max_value);
  }
}

TEST_F(ClapPluginTest, ParamsInfoOutOfRangeFails) {
  clap_param_info_t info;
  EXPECT_FALSE(plugin_->ParamsInfo(100, &info));
}

TEST_F(ClapPluginTest, ParamsGetValueReturnsDefaults) {
  double value;
  
  EXPECT_TRUE(plugin_->ParamsValue(kParamIdThreshold, &value));
  EXPECT_DOUBLE_EQ(value, -0.1);
  
  EXPECT_TRUE(plugin_->ParamsValue(kParamIdOutputLevel, &value));
  EXPECT_DOUBLE_EQ(value, -0.1);
}

TEST_F(ClapPluginTest, ParamsValueToTextFormatsCorrectly) {
  char display[128];
  
  EXPECT_TRUE(plugin_->ParamsValueToText(kParamIdThreshold, -0.1, display, sizeof(display)));
  EXPECT_STREQ(display, "-0.10 dB");
  
  EXPECT_TRUE(plugin_->ParamsValueToText(kParamIdOutputLevel, -0.1, display, sizeof(display)));
  EXPECT_STREQ(display, "-0.10 dB");
}

TEST_F(ClapPluginTest, ParamsTextToValueParsesCorrectly) {
  double value;
  
  EXPECT_TRUE(plugin_->ParamsTextToValue(kParamIdThreshold, "-3.0", &value));
  EXPECT_DOUBLE_EQ(value, -3.0);
  
  EXPECT_TRUE(plugin_->ParamsTextToValue(kParamIdOutputLevel, "-6.0", &value));
  EXPECT_DOUBLE_EQ(value, -6.0);
}

TEST_F(ClapPluginTest, AudioPortsCountReturnsOne) {
  EXPECT_EQ(plugin_->AudioPortsCount(true), 1u);   // Input
  EXPECT_EQ(plugin_->AudioPortsCount(false), 1u);  // Output
}

TEST_F(ClapPluginTest, AudioPortsGetReturnsValidInfo) {
  clap_audio_port_info_t info;
  
  EXPECT_TRUE(plugin_->AudioPortsGet(0, true, &info));
  EXPECT_EQ(info.channel_count, 2u);
  EXPECT_EQ(info.flags, CLAP_AUDIO_PORT_IS_MAIN);
  
  EXPECT_TRUE(plugin_->AudioPortsGet(0, false, &info));
  EXPECT_EQ(info.channel_count, 2u);
  EXPECT_EQ(info.flags, CLAP_AUDIO_PORT_IS_MAIN);
}

TEST_F(ClapPluginTest, AudioPortsGetOutOfRangeFails) {
  clap_audio_port_info_t info;
  EXPECT_FALSE(plugin_->AudioPortsGet(1, true, &info));
}

TEST_F(ClapPluginTest, GetExtensionReturnsValidPointers) {
  EXPECT_NE(plugin_->GetExtension(CLAP_EXT_AUDIO_PORTS), nullptr);
  EXPECT_NE(plugin_->GetExtension(CLAP_EXT_PARAMS), nullptr);
  EXPECT_NE(plugin_->GetExtension(CLAP_EXT_STATE), nullptr);
}

TEST_F(ClapPluginTest, GetExtensionReturnsNullForUnknown) {
  EXPECT_EQ(plugin_->GetExtension("unknown.extension"), nullptr);
}

// Process tests
class ProcessTest : public ::testing::Test {
 protected:
  void SetUp() override {
    host_ = std::make_unique<MockClapHost>();
    plugin_ = std::make_unique<LimiterClap>(host_->Host());
    ASSERT_TRUE(plugin_->Init());
    ASSERT_TRUE(plugin_->Activate(44100.0, 64, 512));
    ASSERT_TRUE(plugin_->StartProcessing());
    
    // Initialize test buffers
    input_left_.resize(kBufferSize, 0.0f);
    input_right_.resize(kBufferSize, 0.0f);
    output_left_.resize(kBufferSize, 0.0f);
    output_right_.resize(kBufferSize, 0.0f);
    
    input_ptrs_[0] = input_left_.data();
    input_ptrs_[1] = input_right_.data();
    output_ptrs_[0] = output_left_.data();
    output_ptrs_[1] = output_right_.data();
    
    input_port_.data32 = input_ptrs_;
    input_port_.data64 = nullptr;
    input_port_.channel_count = 2;
    input_port_.latency = 0;
    input_port_.constant_mask = 0;
    
    output_port_.data32 = output_ptrs_;
    output_port_.data64 = nullptr;
    output_port_.channel_count = 2;
    output_port_.latency = 0;
    output_port_.constant_mask = 0;
    
    process_.steady_time = 0;
    process_.frames_count = kBufferSize;
    process_.transport = nullptr;
    process_.audio_inputs = &input_port_;
    process_.audio_outputs = &output_port_;
    process_.audio_inputs_count = 1;
    process_.audio_outputs_count = 1;
    process_.in_events = nullptr;
    process_.out_events = nullptr;
  }

  void TearDown() override {
    plugin_->StopProcessing();
    plugin_->Deactivate();
    plugin_.reset();
    host_.reset();
  }

  static constexpr size_t kBufferSize = 512;
  
  std::unique_ptr<MockClapHost> host_;
  std::unique_ptr<LimiterClap> plugin_;
  
  std::vector<float> input_left_;
  std::vector<float> input_right_;
  std::vector<float> output_left_;
  std::vector<float> output_right_;
  
  float* input_ptrs_[2];
  float* output_ptrs_[2];
  
  clap_audio_buffer_t input_port_;
  clap_audio_buffer_t output_port_;
  clap_process_t process_;
};

TEST_F(ProcessTest, ProcessSilenceProducesSilence) {
  EXPECT_EQ(plugin_->Process(&process_), CLAP_PROCESS_CONTINUE);
  
  for (size_t i = 0; i < kBufferSize; ++i) {
    EXPECT_FLOAT_EQ(output_left_[i], 0.0f);
    EXPECT_FLOAT_EQ(output_right_[i], 0.0f);
  }
}

TEST_F(ProcessTest, ProcessLowLevelSignalPassesThrough) {
  // Generate low-level signal (-20 dB, well below default -0.1 dB threshold)
  constexpr float kSignalLevel = 0.1f;  // -20 dB
  for (size_t i = 0; i < kBufferSize; ++i) {
    input_left_[i] = kSignalLevel;
    input_right_[i] = kSignalLevel;
  }
  
  EXPECT_EQ(plugin_->Process(&process_), CLAP_PROCESS_CONTINUE);
  
  // Should pass through with minimal change
  for (size_t i = 0; i < kBufferSize; ++i) {
    EXPECT_NEAR(output_left_[i], kSignalLevel, 0.01f);
    EXPECT_NEAR(output_right_[i], kSignalLevel, 0.01f);
  }
}

TEST_F(ProcessTest, ProcessHighLevelSignalIsLimited) {
  // Generate signal above 0 dBFS (would clip without limiting)
  constexpr float kSignalLevel = 1.5f;  // ~3.5 dB
  for (size_t i = 0; i < kBufferSize; ++i) {
    input_left_[i] = kSignalLevel;
    input_right_[i] = kSignalLevel;
  }
  
  EXPECT_EQ(plugin_->Process(&process_), CLAP_PROCESS_CONTINUE);
  
  // After limiting with lookahead, signal should be under threshold
  // Note: First few samples may still be high due to lookahead delay
  for (size_t i = 100; i < kBufferSize; ++i) {
    EXPECT_LE(std::abs(output_left_[i]), 1.01f);   // Allow small tolerance
    EXPECT_LE(std::abs(output_right_[i]), 1.01f);
  }
}

TEST_F(ProcessTest, GainReductionIsApplied) {
  // Generate signal that requires limiting
  constexpr float kSignalLevel = 2.0f;  // ~6 dB
  for (size_t i = 0; i < kBufferSize; ++i) {
    input_left_[i] = kSignalLevel;
    input_right_[i] = kSignalLevel;
  }
  
  plugin_->Process(&process_);
  
  // Check that gain reduction is being reported
  float gain_reduction = plugin_->GetGainReduction();
  EXPECT_LT(gain_reduction, 0.0f);  // Should be negative (reduction)
}

}  // namespace
}  // namespace fast_limiter

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
