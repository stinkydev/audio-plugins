// Copyright 2025
// Basic CLAP plugin tests for Delay

#include "delay_clap.h"

#include <gtest/gtest.h>

#include <cstring>
#include <memory>

using namespace stinky_delay;

namespace {

// Mock CLAP host
struct MockHost {
  clap_host_t host;
  
  MockHost() {
    std::memset(&host, 0, sizeof(host));
    host.clap_version = CLAP_VERSION;
    host.host_data = this;
    host.name = "Test Host";
    host.vendor = "Test";
    host.url = "https://test.com";
    host.version = "1.0.0";
  }
};

class ClapDelayPluginTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mock_host_ = std::make_unique<MockHost>();
    plugin_ = std::make_unique<DelayClap>(&mock_host_->host);
  }

  std::unique_ptr<MockHost> mock_host_;
  std::unique_ptr<DelayClap> plugin_;
};

TEST_F(ClapDelayPluginTest, InitSucceeds) {
  EXPECT_TRUE(plugin_->Init());
}

TEST_F(ClapDelayPluginTest, ActivateSucceeds) {
  EXPECT_TRUE(plugin_->Init());
  EXPECT_TRUE(plugin_->Activate(44100.0, 64, 8192));
}

TEST_F(ClapDelayPluginTest, StartProcessingSucceeds) {
  EXPECT_TRUE(plugin_->Init());
  EXPECT_TRUE(plugin_->Activate(44100.0, 64, 8192));
  EXPECT_TRUE(plugin_->StartProcessing());
}

TEST_F(ClapDelayPluginTest, DeactivateSucceeds) {
  EXPECT_TRUE(plugin_->Init());
  EXPECT_TRUE(plugin_->Activate(44100.0, 64, 8192));
  plugin_->Deactivate();
}

TEST_F(ClapDelayPluginTest, ResetSucceeds) {
  EXPECT_TRUE(plugin_->Init());
  EXPECT_TRUE(plugin_->Activate(44100.0, 64, 8192));
  plugin_->Reset();
}

TEST_F(ClapDelayPluginTest, ParamsCountReturnsCorrectValue) {
  EXPECT_EQ(plugin_->ParamsCount(), 2u);
}

TEST_F(ClapDelayPluginTest, ParamsInfoReturnsValidInfo) {
  clap_param_info_t info;
  
  EXPECT_TRUE(plugin_->ParamsInfo(0, &info));
  EXPECT_STREQ(info.name, "Delay Time");
  EXPECT_EQ(info.min_value, 0.0);
  EXPECT_EQ(info.max_value, 1.0);
  
  EXPECT_TRUE(plugin_->ParamsInfo(1, &info));
  EXPECT_STREQ(info.name, "Mix");
}

TEST_F(ClapDelayPluginTest, ParamsInfoOutOfBoundsReturnsFalse) {
  clap_param_info_t info;
  EXPECT_FALSE(plugin_->ParamsInfo(999, &info));
}

TEST_F(ClapDelayPluginTest, ParamsValueToTextFormatsCorrectly) {
  char display[256];
  
  EXPECT_TRUE(plugin_->ParamsValueToText(0, 0.25, display, sizeof(display)));
  EXPECT_STREQ(display, "500.0 ms");
  
  EXPECT_TRUE(plugin_->ParamsValueToText(1, 0.5, display, sizeof(display)));
  EXPECT_STREQ(display, "50.0%");
}

TEST_F(ClapDelayPluginTest, GetExtensionReturnsValidPointers) {
  EXPECT_NE(plugin_->GetExtension(CLAP_EXT_PARAMS), nullptr);
  EXPECT_NE(plugin_->GetExtension(CLAP_EXT_STATE), nullptr);
  EXPECT_NE(plugin_->GetExtension(CLAP_EXT_AUDIO_PORTS), nullptr);
  EXPECT_EQ(plugin_->GetExtension("invalid.extension"), nullptr);
}

TEST_F(ClapDelayPluginTest, AudioPortsCountReturnsOne) {
  EXPECT_EQ(plugin_->AudioPortsCount(true), 1u);
  EXPECT_EQ(plugin_->AudioPortsCount(false), 1u);
}

TEST_F(ClapDelayPluginTest, AudioPortsGetReturnsValidInfo) {
  clap_audio_port_info_t info;
  
  EXPECT_TRUE(plugin_->AudioPortsGet(0, true, &info));
  EXPECT_STREQ(info.name, "Audio Input");
  EXPECT_EQ(info.channel_count, 2u);
  
  EXPECT_TRUE(plugin_->AudioPortsGet(0, false, &info));
  EXPECT_STREQ(info.name, "Audio Output");
  EXPECT_EQ(info.channel_count, 2u);
}

TEST_F(ClapDelayPluginTest, AudioPortsGetOutOfBoundsReturnsFalse) {
  clap_audio_port_info_t info;
  EXPECT_FALSE(plugin_->AudioPortsGet(1, true, &info));
  EXPECT_FALSE(plugin_->AudioPortsGet(1, false, &info));
}

}  // namespace
