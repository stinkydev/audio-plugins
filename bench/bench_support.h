// Copyright 2025
// Shared support for the plugin throughput benchmarks.
//
// Header-only so each plugin's benchmark is a single translation unit with no
// dependency beyond the plugin's own sources and the CLAP headers. Numbers are
// only meaningful from a Release build; run every benchmark with:
//
//   cmake --build build --config Release --target bench
//
// Each line reports the cost of one frame of stereo audio and the share of one
// core that costs at the stated sample rate. Compare ns/frame between runs on
// the same machine to catch regressions.

#ifndef BENCH_SUPPORT_H_
#define BENCH_SUPPORT_H_

#include <clap/clap.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace bench {

struct Options {
  double seconds = 10.0;  // audio length per run
  int reps = 5;           // runs per measurement; the best is reported
};

// Usage: <Bench> [seconds] [reps]
inline Options ParseArgs(int argc, char** argv) {
  Options opt;
  if (argc > 1) opt.seconds = std::max(std::atof(argv[1]), 0.1);
  if (argc > 2) opt.reps = std::max(std::atoi(argv[2]), 1);
  return opt;
}

inline void PrintHeader(const char* plugin, const Options& opt) {
  std::printf(
      "%s benchmarks: %.1f s of stereo audio per run, best of %d runs\n\n",
      plugin, opt.seconds, opt.reps);
}

// Deterministic test signal: a 440 Hz sine whose amplitude swings between 20%
// and 100% of `peak` twice a second, plus a little noise. Dynamics processors
// are therefore busy for part of each cycle rather than all or none of the
// time.
inline void FillSignal(std::vector<float>& left, std::vector<float>& right,
                       double sample_rate, float peak) {
  constexpr double kPi = 3.14159265358979323846;
  uint32_t seed = 0x12345678u;
  for (size_t i = 0; i < left.size(); ++i) {
    seed = seed * 1664525u + 1013904223u;
    const float noise =
        (static_cast<float>(seed >> 8) / 16777216.0f - 0.5f) * 0.05f;
    const double t = static_cast<double>(i) / sample_rate;
    const float am =
        0.6f + 0.4f * static_cast<float>(std::sin(2.0 * kPi * 0.5 * t));
    const float a = peak * am;
    left[i] = a * static_cast<float>(std::sin(2.0 * kPi * 440.0 * t)) + noise;
    right[i] =
        a * static_cast<float>(std::sin(2.0 * kPi * 440.0 * t + 0.3)) + noise;
  }
}

// Accumulates a few output samples so the compiler cannot discard the work.
inline double& Sink() {
  static double sink = 0.0;
  return sink;
}

inline void PrintFooter() { std::printf("\n(checksum %g)\n", Sink()); }

inline void Report(const char* name, double sample_rate, size_t block,
                   size_t frames, double best_seconds) {
  const double ns_per_frame =
      best_seconds / static_cast<double>(frames) * 1e9;
  const double audio_seconds = static_cast<double>(frames) / sample_rate;
  const double x_realtime = audio_seconds / best_seconds;
  std::printf(
      "%-28s %6.1f kHz  block %4zu  %7.1f ns/frame  %7.1f us/block  "
      "%7.0fx realtime  %6.3f%% of one core\n",
      name, sample_rate / 1000.0, block, ns_per_frame,
      ns_per_frame * static_cast<double>(block) / 1e3, x_realtime,
      100.0 / x_realtime);
}

// Times `process(left, right, frames)`, which works in place, over the test
// signal in `block`-frame blocks.
template <typename Fn>
inline void RunCore(const char* name, double sample_rate, size_t block,
                    const Options& opt, Fn&& process) {
  const size_t frames = static_cast<size_t>(opt.seconds * sample_rate);
  std::vector<float> in_left(frames), in_right(frames);
  FillSignal(in_left, in_right, sample_rate, 1.0f);
  std::vector<float> left(frames), right(frames);

  double best = 1e300;
  for (int rep = 0; rep < opt.reps; ++rep) {
    std::copy(in_left.begin(), in_left.end(), left.begin());
    std::copy(in_right.begin(), in_right.end(), right.begin());
    const auto t0 = std::chrono::steady_clock::now();
    for (size_t i = 0; i < frames; i += block) {
      const size_t n = std::min(block, frames - i);
      process(left.data() + i, right.data() + i, n);
    }
    const auto t1 = std::chrono::steady_clock::now();
    best = std::min(best, std::chrono::duration<double>(t1 - t0).count());
    Sink() += left[frames / 2] + right[frames - 1];
  }
  Report(name, sample_rate, block, frames, best);
}

// A host that offers no extensions and ignores all requests.
inline clap_host_t NullHost() {
  clap_host_t host{};
  host.clap_version = CLAP_VERSION;
  host.name = "bench";
  host.vendor = "bench";
  host.url = "";
  host.version = "0";
  host.get_extension = [](const clap_host_t*, const char*) -> const void* {
    return nullptr;
  };
  host.request_restart = [](const clap_host_t*) {};
  host.request_process = [](const clap_host_t*) {};
  host.request_callback = [](const clap_host_t*) {};
  return host;
}

// Drives a CLAP wrapper's Process() the way a host does: default parameters,
// one stereo input and a separate stereo output buffer per block. This covers
// the per-block parameter sync and the input-to-output copy as well as the
// DSP itself.
template <typename Plugin>
inline void RunClap(const char* name, double sample_rate, size_t block,
                    const Options& opt) {
  const clap_host_t host = NullHost();
  Plugin plugin(&host);
  if (!plugin.Init() ||
      !plugin.Activate(sample_rate, static_cast<uint32_t>(block),
                       static_cast<uint32_t>(block)) ||
      !plugin.StartProcessing()) {
    std::printf("%-28s failed to initialise\n", name);
    return;
  }

  const size_t frames = static_cast<size_t>(opt.seconds * sample_rate);
  std::vector<float> in_left(frames), in_right(frames);
  FillSignal(in_left, in_right, sample_rate, 1.0f);
  std::vector<float> out_left(frames), out_right(frames);

  float* in_ptrs[2] = {nullptr, nullptr};
  float* out_ptrs[2] = {nullptr, nullptr};
  clap_audio_buffer_t in{};
  in.data32 = in_ptrs;
  in.channel_count = 2;
  clap_audio_buffer_t out{};
  out.data32 = out_ptrs;
  out.channel_count = 2;
  clap_process_t process{};
  process.audio_inputs = &in;
  process.audio_inputs_count = 1;
  process.audio_outputs = &out;
  process.audio_outputs_count = 1;

  double best = 1e300;
  for (int rep = 0; rep < opt.reps; ++rep) {
    const auto t0 = std::chrono::steady_clock::now();
    for (size_t i = 0; i < frames; i += block) {
      const size_t n = std::min(block, frames - i);
      in_ptrs[0] = in_left.data() + i;
      in_ptrs[1] = in_right.data() + i;
      out_ptrs[0] = out_left.data() + i;
      out_ptrs[1] = out_right.data() + i;
      process.frames_count = static_cast<uint32_t>(n);
      process.steady_time = static_cast<int64_t>(i);
      plugin.Process(&process);
    }
    const auto t1 = std::chrono::steady_clock::now();
    best = std::min(best, std::chrono::duration<double>(t1 - t0).count());
    Sink() += out_left[frames / 2] + out_right[frames - 1];
  }
  Report(name, sample_rate, block, frames, best);

  plugin.StopProcessing();
  plugin.Deactivate();
}

}  // namespace bench

#endif  // BENCH_SUPPORT_H_
