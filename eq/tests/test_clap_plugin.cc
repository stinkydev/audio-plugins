// Copyright 2025
// End-to-end tests for CLAP EQ plugin

#include "eq_clap.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>
#include <memory>
#include <vector>

namespace fast_eq {
namespace {

using ::testing::NotNull;

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

class ClapEqPluginTest : public ::testing::Test {
 protected:
  void SetUp() override {
    host_ = std::make_unique<MockClapHost>();
    plugin_ = std::make_unique<EqClap>(host_->Host());
    ASSERT_TRUE(plugin_->Init());
  }

  void TearDown() override {
    plugin_.reset();
    host_.reset();
  }

  std::unique_ptr<MockClapHost> host_;
  std::unique_ptr<EqClap> plugin_;
};

TEST_F(ClapEqPluginTest, InitSucceeds) {
  EXPECT_THAT(plugin_, NotNull());
}

TEST_F(ClapEqPluginTest, ActivateSucceeds) {
  EXPECT_TRUE(plugin_->Activate(44100.0, 64, 512));
}

TEST_F(ClapEqPluginTest, StartProcessingSucceeds) {
  plugin_->Activate(44100.0, 64, 512);
  EXPECT_TRUE(plugin_->StartProcessing());
}

TEST_F(ClapEqPluginTest, DeactivateSucceeds) {
  plugin_->Activate(44100.0, 64, 512);
  plugin_->Deactivate();
  SUCCEED();
}

TEST_F(ClapEqPluginTest, ResetSucceeds) {
  plugin_->Activate(44100.0, 64, 512);
  plugin_->Reset();
  SUCCEED();
}

TEST_F(ClapEqPluginTest, ParamsCountReturnsCorrectValue) {
  EXPECT_EQ(plugin_->ParamsCount(), kParamIdCount);
}

TEST_F(ClapEqPluginTest, ParamsInfoReturnsValidInfo) {
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

TEST_F(ClapEqPluginTest, ParamsInfoOutOfBoundsReturnsFalse) {
  clap_param_info_t info;
  EXPECT_FALSE(plugin_->ParamsInfo(kParamIdCount, &info));
}

TEST_F(ClapEqPluginTest, ParamsGetValueReturnsDefault) {
  double value;
  
  EXPECT_TRUE(plugin_->ParamsValue(kParamIdBand1Freq, &value));
  EXPECT_DOUBLE_EQ(value, 100.0);
  
  EXPECT_TRUE(plugin_->ParamsValue(kParamIdBand2Freq, &value));
  EXPECT_DOUBLE_EQ(value, 500.0);
  
  EXPECT_TRUE(plugin_->ParamsValue(kParamIdBand3Freq, &value));
  EXPECT_DOUBLE_EQ(value, 2000.0);
  
  EXPECT_TRUE(plugin_->ParamsValue(kParamIdBand4Freq, &value));
  EXPECT_DOUBLE_EQ(value, 8000.0);
}

TEST_F(ClapEqPluginTest, ParamsValueToTextFormatsCorrectly) {
  char display[256];
  
  // Test frequency formatting (Hz)
  EXPECT_TRUE(plugin_->ParamsValueToText(kParamIdBand1Freq, 100.0, display, sizeof(display)));
  EXPECT_STREQ(display, "100.0 Hz");
  
  // Test frequency formatting (kHz)
  EXPECT_TRUE(plugin_->ParamsValueToText(kParamIdBand1Freq, 1000.0, display, sizeof(display)));
  EXPECT_STREQ(display, "1.00 kHz");
  
  // Test gain formatting
  EXPECT_TRUE(plugin_->ParamsValueToText(kParamIdBand1Gain, 6.0, display, sizeof(display)));
  EXPECT_STREQ(display, "6.0 dB");
  
  // Test Q formatting
  EXPECT_TRUE(plugin_->ParamsValueToText(kParamIdBand1Q, 1.5, display, sizeof(display)));
  EXPECT_STREQ(display, "1.50");
  
  // Test filter type formatting
  EXPECT_TRUE(plugin_->ParamsValueToText(kParamIdBand1Type, 
              static_cast<double>(FilterType::kHighCut), display, sizeof(display)));
  EXPECT_STREQ(display, "High Cut");
  
  EXPECT_TRUE(plugin_->ParamsValueToText(kParamIdBand1Type,
              static_cast<double>(FilterType::kLowCut), display, sizeof(display)));
  EXPECT_STREQ(display, "Low Cut");
}

TEST_F(ClapEqPluginTest, AudioPortsCountReturnsOne) {
  EXPECT_EQ(plugin_->AudioPortsCount(true), 1u);
  EXPECT_EQ(plugin_->AudioPortsCount(false), 1u);
}

TEST_F(ClapEqPluginTest, AudioPortsGetReturnsValidInfo) {
  clap_audio_port_info_t info;
  
  EXPECT_TRUE(plugin_->AudioPortsGet(0, true, &info));
  EXPECT_EQ(info.channel_count, 2u);
  EXPECT_EQ(info.flags, CLAP_AUDIO_PORT_IS_MAIN);
  
  EXPECT_TRUE(plugin_->AudioPortsGet(0, false, &info));
  EXPECT_EQ(info.channel_count, 2u);
  EXPECT_EQ(info.flags, CLAP_AUDIO_PORT_IS_MAIN);
}

TEST_F(ClapEqPluginTest, AudioPortsGetOutOfBoundsReturnsFalse) {
  clap_audio_port_info_t info;
  EXPECT_FALSE(plugin_->AudioPortsGet(1, true, &info));
  EXPECT_FALSE(plugin_->AudioPortsGet(1, false, &info));
}

TEST_F(ClapEqPluginTest, GetExtensionReturnsValidPointers) {
  EXPECT_THAT(plugin_->GetExtension(CLAP_EXT_PARAMS), NotNull());
  EXPECT_THAT(plugin_->GetExtension(CLAP_EXT_AUDIO_PORTS), NotNull());
  EXPECT_THAT(plugin_->GetExtension(CLAP_EXT_STATE), NotNull());
  EXPECT_EQ(plugin_->GetExtension("unknown_extension"), nullptr);
}

TEST_F(ClapEqPluginTest, StateSaveAndLoadRoundTrip) {
  // Set some parameter values
  double test_value = 2000.0;
  plugin_->ParamsValue(kParamIdBand1Freq, &test_value);
  
  // Create a simple stream implementation for testing
  struct MemoryStream {
    std::vector<uint8_t> data;
    size_t read_pos = 0;
  };
  
  MemoryStream stream;
  
  clap_ostream_t ostream;
  ostream.ctx = &stream;
  ostream.write = [](const clap_ostream_t* s, const void* buffer, uint64_t size) -> int64_t {
    auto* stream = static_cast<MemoryStream*>(s->ctx);
    const uint8_t* bytes = static_cast<const uint8_t*>(buffer);
    stream->data.insert(stream->data.end(), bytes, bytes + size);
    return size;
  };
  
  EXPECT_TRUE(plugin_->StateSave(&ostream));
  EXPECT_GT(stream.data.size(), 0u);
  
  clap_istream_t istream;
  istream.ctx = &stream;
  istream.read = [](const clap_istream_t* s, void* buffer, uint64_t size) -> int64_t {
    auto* stream = static_cast<MemoryStream*>(s->ctx);
    const size_t available = stream->data.size() - stream->read_pos;
    const size_t to_read = std::min(static_cast<size_t>(size), available);
    std::memcpy(buffer, stream->data.data() + stream->read_pos, to_read);
    stream->read_pos += to_read;
    return to_read;
  };
  
  // Create new plugin and load state
  auto plugin2 = std::make_unique<EqClap>(host_->Host());
  plugin2->Init();
  EXPECT_TRUE(plugin2->StateLoad(&istream));
}

TEST_F(ClapEqPluginTest, ProcessStereoAudioDoesNotCrash) {
  plugin_->Activate(44100.0, 64, 512);
  plugin_->StartProcessing();
  
  std::vector<float> left_in(512, 0.0f);
  std::vector<float> right_in(512, 0.0f);
  std::vector<float> left_out(512);
  std::vector<float> right_out(512);
  
  float* in_ptrs[] = {left_in.data(), right_in.data()};
  float* out_ptrs[] = {left_out.data(), right_out.data()};
  
  clap_audio_buffer_t input;
  input.data32 = in_ptrs;
  input.data64 = nullptr;
  input.channel_count = 2;
  input.latency = 0;
  input.constant_mask = 0;
  
  clap_audio_buffer_t output;
  output.data32 = out_ptrs;
  output.data64 = nullptr;
  output.channel_count = 2;
  output.latency = 0;
  output.constant_mask = 0;
  
  clap_process_t process;
  std::memset(&process, 0, sizeof(process));
  process.frames_count = 512;
  process.audio_inputs = &input;
  process.audio_inputs_count = 1;
  process.audio_outputs = &output;
  process.audio_outputs_count = 1;
  
  EXPECT_EQ(plugin_->Process(&process), CLAP_PROCESS_CONTINUE);
}

}  // namespace
}  // namespace fast_eq
