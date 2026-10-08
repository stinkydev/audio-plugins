// Copyright 2025
// Throughput benchmark for the compressor: the DSP core on its own and the
// CLAP wrapper's Process() as a host would call it.
//
//   CompressorBench [seconds] [reps]

#include "bench_support.h"
#include "compressor_clap.h"
#include "compressor_processor.h"

int main(int argc, char** argv) {
  const bench::Options opt = bench::ParseArgs(argc, argv);
  bench::PrintHeader("Compressor", opt);

  // Core: 4:1 with a soft knee and auto makeup against a 0 dBFS signal, so
  // the gain computer and both smoothers are busy throughout.
  {
    fast_compressor::CompressorProcessor processor;
    processor.Initialize(48000.0);
    fast_compressor::CompressorParams params;
    params.threshold_db = -20.0f;
    params.ratio = 4.0f;
    params.attack_ms = 5.0f;
    params.release_ms = 50.0f;
    params.knee_db = 6.0f;
    params.auto_makeup = true;
    processor.SetParams(params);
    bench::RunCore("core, 4:1 soft knee", 48000.0, 512, opt,
                   [&](float* left, float* right, size_t frames) {
                     processor.ProcessStereo(left, right, frames);
                   });
  }

  // Wrapper with default parameters (-20 dB threshold, so still compressing).
  // Two block sizes expose the fixed per-block cost of the parameter sync.
  bench::RunClap<fast_compressor::CompressorClap>("CLAP Process(), defaults",
                                                  48000.0, 64, opt);
  bench::RunClap<fast_compressor::CompressorClap>("CLAP Process(), defaults",
                                                  48000.0, 512, opt);

  bench::PrintFooter();
  return 0;
}
