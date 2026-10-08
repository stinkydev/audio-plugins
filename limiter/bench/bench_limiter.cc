// Copyright 2025
// Throughput benchmark for the limiter: the DSP core at each oversampling
// tier and the CLAP wrapper's Process() as a host would call it.
//
//   LimiterBench [seconds] [reps]

#include <cstdio>

#include "bench_support.h"
#include "limiter_clap.h"
#include "limiter_processor.h"

int main(int argc, char** argv) {
  const bench::Options opt = bench::ParseArgs(argc, argv);
  bench::PrintHeader("Limiter", opt);

  // Core at the three oversampling tiers: 4x true-peak below 64 kHz, 2x up
  // to 128 kHz, sample-peak above. Ceiling -1 dBTP against a 0 dBFS signal,
  // so it limits during the louder half of each modulation cycle.
  for (double sample_rate : {48000.0, 96000.0, 192000.0}) {
    fast_limiter::LimiterProcessor processor;
    processor.Initialize(sample_rate);
    fast_limiter::LimiterParams params;
    params.threshold_db = -1.0f;
    params.output_level_db = -1.0f;
    processor.SetParams(params);
    char name[64];
    std::snprintf(name, sizeof(name), "core, %dx true-peak",
                  processor.GetOversamplingFactor());
    bench::RunCore(name, sample_rate, 512, opt,
                   [&](float* left, float* right, size_t frames) {
                     processor.ProcessStereo(left, right, frames);
                   });
  }

  // Wrapper with default parameters (-1 dB ceiling, so still limiting).
  // Two block sizes expose the fixed per-block cost of the parameter sync.
  bench::RunClap<fast_limiter::LimiterClap>("CLAP Process(), defaults",
                                            48000.0, 64, opt);
  bench::RunClap<fast_limiter::LimiterClap>("CLAP Process(), defaults",
                                            48000.0, 512, opt);

  bench::PrintFooter();
  return 0;
}
