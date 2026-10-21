// Copyright 2025
// Throughput benchmark for the EQ: the DSP core with all four bands active
// and the CLAP wrapper's Process() as a host would call it.
//
//   EqBench [seconds] [reps]

#include "bench_support.h"
#include "eq_clap.h"
#include "eq_processor.h"

int main(int argc, char** argv) {
  const bench::Options opt = bench::ParseArgs(argc, argv);
  bench::PrintHeader("EQ", opt);

  // Core: every band enabled with non-zero gain, plus output trim.
  {
    fast_eq::EqProcessor processor;
    processor.Initialize(48000.0);
    fast_eq::EqParams params;
    params.bands[0] = {fast_eq::FilterType::kLowShelf, 100.0f, 3.0f, 0.707f,
                       true};
    params.bands[1] = {fast_eq::FilterType::kBell, 500.0f, -2.0f, 1.0f, true};
    params.bands[2] = {fast_eq::FilterType::kBell, 2000.0f, 4.0f, 1.0f, true};
    params.bands[3] = {fast_eq::FilterType::kHighShelf, 8000.0f, 2.0f, 0.707f,
                       true};
    params.output_gain_db = -1.0f;
    processor.SetParams(params);
    bench::RunCore("core, 4 bands", 48000.0, 512, opt,
                   [&](float* left, float* right, size_t frames) {
                     processor.ProcessStereo(left, right, frames);
                   });
  }

  // Wrapper with default parameters (all bands enabled at 0 dB). Two block
  // sizes expose the fixed per-block cost of the parameter sync.
  bench::RunClap<fast_eq::EqClap>("CLAP Process(), defaults", 48000.0, 64,
                                  opt);
  bench::RunClap<fast_eq::EqClap>("CLAP Process(), defaults", 48000.0, 512,
                                  opt);

  bench::PrintFooter();
  return 0;
}
