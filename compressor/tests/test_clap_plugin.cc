// Copyright 2025
// End-to-end tests for CLAP plugin

#include "compressor_clap.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>
#include <memory>
#include <vector>

namespace fast_compressor {
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

class ClapPluginTest : public ::testing::Test {
 protected:
  void SetUp() override {
    host_ = std::make_unique<MockClapHost>();
    plugin_ = std::make_unique<CompressorClap>(host_->Host());
    ASSERT_TRUE(plugin_->Init());
  }

  void TearDown() override {
    plugin_.reset();
    host_.reset();
  }

  std::unique_ptr<MockClapHost> host_;
  std::unique_ptr<CompressorClap> plugin_;
};

TEST_F(ClapPluginTest, InitSucceeds) {
  EXPECT_THAT(plugin_, NotNull());
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
  EXPECT_EQ(plugin_->ParamsCount(), 7u);
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
  clap_param_info_t info;
  
  // Values are normalized (0-1), so we check against the default_value from ParamsInfo
  EXPECT_TRUE(plugin_->ParamsInfo(kParamIdThreshold, &info));
  EXPECT_TRUE(plugin_->ParamsValue(kParamIdThreshold, &value));
  EXPECT_NEAR(value, info.default_value, 0.0001);
  
  EXPECT_TRUE(plugin_->ParamsInfo(kParamIdRatio, &info));
  EXPECT_TRUE(plugin_->ParamsValue(kParamIdRatio, &value));
  EXPECT_NEAR(value, info.default_value, 0.0001);
  
  EXPECT_TRUE(plugin_->ParamsInfo(kParamIdAttack, &info));
  EXPECT_TRUE(plugin_->ParamsValue(kParamIdAttack, &value));
  EXPECT_NEAR(value, info.default_value, 0.0001);
  
  EXPECT_TRUE(plugin_->ParamsInfo(kParamIdRelease, &info));
  EXPECT_TRUE(plugin_->ParamsValue(kParamIdRelease, &value));
  EXPECT_NEAR(value, info.default_value, 0.0001);
  
  EXPECT_TRUE(plugin_->ParamsInfo(kParamIdKnee, &info));
  EXPECT_TRUE(plugin_->ParamsValue(kParamIdKnee, &value));
  EXPECT_NEAR(value, info.default_value, 0.0001);
  
  EXPECT_TRUE(plugin_->ParamsInfo(kParamIdMakeupGain, &info));
  EXPECT_TRUE(plugin_->ParamsValue(kParamIdMakeupGain, &value));
  EXPECT_NEAR(value, info.default_value, 0.0001);
}

TEST_F(ClapPluginTest, ParamsValueToTextFormatsCorrectly) {
  char display[128];
  clap_param_info_t info;
  
  // ParamsValueToText takes normalized values (0-1) and converts them to text
  EXPECT_TRUE(plugin_->ParamsInfo(kParamIdThreshold, &info));
  EXPECT_TRUE(plugin_->ParamsValueToText(kParamIdThreshold, info.default_value, display, sizeof(display)));
  EXPECT_STREQ(display, "-20.0 dB");
  
  EXPECT_TRUE(plugin_->ParamsInfo(kParamIdRatio, &info));
  EXPECT_TRUE(plugin_->ParamsValueToText(kParamIdRatio, info.default_value, display, sizeof(display)));
  EXPECT_STREQ(display, "4.0:1");
  
  EXPECT_TRUE(plugin_->ParamsInfo(kParamIdAttack, &info));
  EXPECT_TRUE(plugin_->ParamsValueToText(kParamIdAttack, info.default_value, display, sizeof(display)));
  EXPECT_STREQ(display, "5.0 ms");
  
  EXPECT_TRUE(plugin_->ParamsInfo(kParamIdRelease, &info));
  EXPECT_TRUE(plugin_->ParamsValueToText(kParamIdRelease, info.default_value, display, sizeof(display)));
  EXPECT_STREQ(display, "50.0 ms");
}

TEST_F(ClapPluginTest, ParamsTextToValueParsesCorrectly) {
  double value;
  
  // ParamsTextToValue parses text and returns normalized values (0-1)
  EXPECT_TRUE(plugin_->ParamsTextToValue(kParamIdThreshold, "-30.0", &value));
  EXPECT_GE(value, 0.0);
  EXPECT_LE(value, 1.0);
  
  EXPECT_TRUE(plugin_->ParamsTextToValue(kParamIdRatio, "8.0", &value));
  EXPECT_GE(value, 0.0);
  EXPECT_LE(value, 1.0);
  
  EXPECT_TRUE(plugin_->ParamsTextToValue(kParamIdAttack, "10.5", &value));
  EXPECT_GE(value, 0.0);
  EXPECT_LE(value, 1.0);
}

TEST_F(ClapPluginTest, GetExtensionReturnsParams) {
  const void* ext = plugin_->GetExtension(CLAP_EXT_PARAMS);
  EXPECT_THAT(ext, NotNull());
}

TEST_F(ClapPluginTest, GetExtensionReturnsState) {
  const void* ext = plugin_->GetExtension(CLAP_EXT_STATE);
  EXPECT_THAT(ext, NotNull());
}

TEST_F(ClapPluginTest, GetExtensionReturnsNullForUnknown) {
  const void* ext = plugin_->GetExtension("unknown.extension");
  EXPECT_EQ(ext, nullptr);
}

TEST_F(ClapPluginTest, ProcessSilenceWithNoInput) {
  plugin_->Activate(44100.0, 64, 512);
  plugin_->StartProcessing();

  // Setup process data
  constexpr uint32_t frame_count = 64;
  std::vector<float> in_left(frame_count, 0.0f);
  std::vector<float> in_right(frame_count, 0.0f);
  std::vector<float> out_left(frame_count, 0.0f);
  std::vector<float> out_right(frame_count, 0.0f);

  float* in_ptrs[] = {in_left.data(), in_right.data()};
  float* out_ptrs[] = {out_left.data(), out_right.data()};

  clap_audio_buffer_t input = {
      in_ptrs,
      nullptr,
      2,
      frame_count,
      0,
  };

  clap_audio_buffer_t output = {
      out_ptrs,
      nullptr,
      2,
      frame_count,
      0,
  };

  clap_process_t process = {};
  process.frames_count = frame_count;
  process.audio_inputs = &input;
  process.audio_inputs_count = 1;
  process.audio_outputs = &output;
  process.audio_outputs_count = 1;
  process.in_events = nullptr;
  process.out_events = nullptr;

  clap_process_status status = plugin_->Process(&process);
  EXPECT_EQ(status, CLAP_PROCESS_CONTINUE);

  // Output should be silence
  for (size_t i = 0; i < frame_count; ++i) {
    EXPECT_FLOAT_EQ(out_left[i], 0.0f);
    EXPECT_FLOAT_EQ(out_right[i], 0.0f);
  }
}

TEST_F(ClapPluginTest, ProcessModifiesSignal) {
  plugin_->Activate(44100.0, 64, 512);
  plugin_->StartProcessing();

  constexpr uint32_t frame_count = 64;
  std::vector<float> in_left(frame_count, 0.5f);  // Loud signal
  std::vector<float> in_right(frame_count, 0.5f);
  std::vector<float> out_left(frame_count, 0.0f);
  std::vector<float> out_right(frame_count, 0.0f);

  float* in_ptrs[] = {in_left.data(), in_right.data()};
  float* out_ptrs[] = {out_left.data(), out_right.data()};

  clap_audio_buffer_t input = {
      in_ptrs,
      nullptr,
      2,
      frame_count,
      0,
  };

  clap_audio_buffer_t output = {
      out_ptrs,
      nullptr,
      2,
      frame_count,
      0,
  };

  clap_process_t process = {};
  process.frames_count = frame_count;
  process.audio_inputs = &input;
  process.audio_inputs_count = 1;
  process.audio_outputs = &output;
  process.audio_outputs_count = 1;
  process.in_events = nullptr;
  process.out_events = nullptr;

  clap_process_status status = plugin_->Process(&process);
  EXPECT_EQ(status, CLAP_PROCESS_CONTINUE);

  // Output should be compressed (less than input due to default threshold of -20dB)
  bool compressed = false;
  for (size_t i = 10; i < frame_count; ++i) {  // Skip initial samples
    if (out_left[i] < 0.5f && out_right[i] < 0.5f) {
      compressed = true;
      break;
    }
  }
  EXPECT_TRUE(compressed);
}

TEST_F(ClapPluginTest, ProcessWithNoInputsReturnsSleep) {
  plugin_->Activate(44100.0, 64, 512);
  plugin_->StartProcessing();

  clap_process_t process = {};
  process.frames_count = 64;
  process.audio_inputs_count = 0;
  process.audio_outputs_count = 0;

  clap_process_status status = plugin_->Process(&process);
  EXPECT_EQ(status, CLAP_PROCESS_SLEEP);
}

TEST_F(ClapPluginTest, StateSaveAndLoadRoundTrip) {
  // Simple stream implementation for testing
  struct TestStream {
    std::vector<uint8_t> data;
    size_t read_pos = 0;
  };

  TestStream stream;

  clap_ostream_t ostream = {
      &stream,
      [](const clap_ostream_t* s, const void* buffer, uint64_t size) -> int64_t {
        auto* ts = static_cast<TestStream*>(s->ctx);
        const uint8_t* bytes = static_cast<const uint8_t*>(buffer);
        ts->data.insert(ts->data.end(), bytes, bytes + size);
        return size;
      },
  };

  clap_istream_t istream = {
      &stream,
      [](const clap_istream_t* s, void* buffer, uint64_t size) -> int64_t {
        auto* ts = static_cast<TestStream*>(s->ctx);
        if (ts->read_pos + size > ts->data.size()) {
          return -1;
        }
        std::memcpy(buffer, ts->data.data() + ts->read_pos, size);
        ts->read_pos += size;
        return size;
      },
  };

  // Save state
  EXPECT_TRUE(plugin_->StateSave(&ostream));
  EXPECT_GT(stream.data.size(), 0u);

  // Create new plugin and load state
  auto plugin2 = std::make_unique<CompressorClap>(host_->Host());
  ASSERT_TRUE(plugin2->Init());
  EXPECT_TRUE(plugin2->StateLoad(&istream));

  // Verify parameters match
  for (uint32_t i = 0; i < plugin_->ParamsCount(); ++i) {
    double value1, value2;
    plugin_->ParamsValue(i, &value1);
    plugin2->ParamsValue(i, &value2);
    EXPECT_DOUBLE_EQ(value1, value2);
  }
}

TEST_F(ClapPluginTest, ClapPluginStructureIsValid) {
  const clap_plugin_t* clap_plugin = plugin_->ClapPlugin();
  EXPECT_THAT(clap_plugin, NotNull());
  EXPECT_THAT(clap_plugin->plugin_data, NotNull());
  EXPECT_THAT(clap_plugin->init, NotNull());
  EXPECT_THAT(clap_plugin->destroy, NotNull());
  EXPECT_THAT(clap_plugin->activate, NotNull());
  EXPECT_THAT(clap_plugin->deactivate, NotNull());
  EXPECT_THAT(clap_plugin->start_processing, NotNull());
  EXPECT_THAT(clap_plugin->stop_processing, NotNull());
  EXPECT_THAT(clap_plugin->reset, NotNull());
  EXPECT_THAT(clap_plugin->process, NotNull());
  EXPECT_THAT(clap_plugin->get_extension, NotNull());
}

TEST_F(ClapPluginTest, MultipleProcessCallsWork) {
  plugin_->Activate(44100.0, 64, 512);
  plugin_->StartProcessing();

  constexpr uint32_t frame_count = 64;
  std::vector<float> in_left(frame_count, 0.3f);
  std::vector<float> in_right(frame_count, 0.3f);
  std::vector<float> out_left(frame_count, 0.0f);
  std::vector<float> out_right(frame_count, 0.0f);

  float* in_ptrs[] = {in_left.data(), in_right.data()};
  float* out_ptrs[] = {out_left.data(), out_right.data()};

  clap_audio_buffer_t input = {in_ptrs, nullptr, 2, frame_count, 0};
  clap_audio_buffer_t output = {out_ptrs, nullptr, 2, frame_count, 0};

  clap_process_t process = {};
  process.frames_count = frame_count;
  process.audio_inputs = &input;
  process.audio_inputs_count = 1;
  process.audio_outputs = &output;
  process.audio_outputs_count = 1;

  // Process multiple times
  for (int i = 0; i < 10; ++i) {
    clap_process_status status = plugin_->Process(&process);
    EXPECT_EQ(status, CLAP_PROCESS_CONTINUE);
  }

  SUCCEED();
}

}  // namespace
}  // namespace fast_compressor
