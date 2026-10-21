// Copyright 2025
// Throughput benchmark for the delay: the DSP core and the CLAP wrapper's
// Process() as a host would call it.
//
//   DelayBench [seconds] [reps]

#include <cstdint>

#include "bench_support.h"
#include "delay_clap.h"
#include "delay_processor.h"

int main(int argc, char** argv) {
  const bench::Options opt = bench::ParseArgs(argc, argv);
  bench::PrintHeader("Delay", opt);

  // Core: 300 ms at 50% mix.
  {
    stinky_delay::DelayProcessor processor;
    processor.Initialize(48000.0);
    stinky_delay::DelayParams params;
    params.delay_time_ms = 300.0f;
    params.mix = 0.5f;
    processor.SetParams(params);
    bench::RunCore("core, 300 ms, 50% mix", 48000.0, 512, opt,
                   [&](float* left, float* right, size_t frames) {
                     processor.ProcessStereo(left, right,
                                             static_cast<uint32_t>(frames));
                   });
  }

  // Wrapper with default parameters. Two block sizes expose the fixed
  // per-block cost of the parameter sync.
  bench::RunClap<stinky_delay::DelayClap>("CLAP Process(), defaults", 48000.0,
                                          64, opt);
  bench::RunClap<stinky_delay::DelayClap>("CLAP Process(), defaults", 48000.0,
                                          512, opt);

  bench::PrintFooter();
  return 0;
}
