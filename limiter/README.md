# Stinky Limiter

A true-peak brickwall limiter CLAP plugin for live production. It is designed to
sit last in the chain on ambience and full-mix buses in live sports, where the
job is to never let a peak through, stay stable on a roaring crowd, and add a
fixed, known delay.

## Features

- **Guaranteed sample-peak ceiling**: enforced by construction, not by tuning.
  No sample leaves the plugin above the output level.
- **True-peak detection**: peaks are measured on a 4x oversampled signal using
  the ITU-R BS.1770-4 interpolation filter, so inter-sample overs that would
  clip a converter or an encoder are caught (2x at 88.2/96 kHz). This is a
  tight bound rather than a guarantee: the output's true peak, as a BS.1770
  meter measures it, stays within 0.01 dB of the ceiling. See "The ceiling
  contract" below.
- **5 ms lookahead**: gain reduction ramps in linearly over 5 ms ahead of the
  peak, so transients are caught without clicks.
- **Dual-stage release**: an 80 ms fast stage handles isolated hits (whistles,
  ball strikes, horns). A slow stage with a 200 ms build-up and 500 ms recovery
  takes over when limiting is sustained, so crowd roar is held at a stable
  level instead of fluttering.
- **Linked stereo**: one gain for both channels keeps the image steady.
- **Reported latency**: the plugin exposes its delay through the CLAP latency
  extension so the host can compensate.
- **Realtime safe**: no allocations or locks in the audio path, and no
  per-sample logarithms (one `log10` per block feeds the gain-reduction meter).

## Parameters

### Threshold
- **Range**: -60.0 dB to 0.0 dB
- **Default**: -1.0 dB
- **Description**: The ceiling. Anything above this is limited: sample peaks
  never exceed it and true peaks stay within 0.01 dB of it.

### Output Level
- **Range**: -60.0 dB to 0.0 dB
- **Default**: -1.0 dB
- **Description**: The final output ceiling. The limited signal is scaled by
  the difference between output level and threshold, so this is the level the
  output never exceeds. Leave it equal to threshold for a plain limiter, or set
  it lower to deliver extra headroom.

The default of -1 dBTP is the EBU R128 permitted maximum. For an ATSC A/85
chain use -2 dB. If the downstream chain still runs on an EBU R68 style PPM
alignment, set both parameters to the permitted maximum level of that chain
(typically -9 dBFS). Where a QC meter must never read above the target, set
the ceiling 0.1 dB below it; the true-peak bound is well inside that margin.

## Building

### Requirements
- CMake 3.20 or later
- C++20 compatible compiler (MSVC, GCC, Clang)

### Build Instructions

```bash
# Configure
cmake -B build -DBUILD_LIMITER=ON

# Build
cmake --build build --config Release

# The plugin will be at: build/limiter/Release/StinkyLimiter.clap
```

### Build Options
- `BUILD_LIMITER`: Enable building the limiter plugin (default: ON)
- `BUILD_TESTS`: Build unit tests (default: ON)
- `ENABLE_SIMD`: AVX2 code generation (default: ON). The true-peak detector
  has a hand-written AVX2 path that evaluates all four interpolation phases of
  both channels with one 8-lane fused multiply-add per filter tap; the rest of
  the limiter is plain C++. With it off, a portable loop that sums in the same
  order is used instead, so results differ only by fused-multiply-add rounding.

### Tests

```bash
cmake --build build --config Release --target LimiterTests
./build/limiter/Release/LimiterTests
```

The processor tests run at 44.1, 48, 96 and 192 kHz and check that the reported
latency equals the measured delay, that no sample of any test signal exceeds the
ceiling, that inter-sample peaks are detected, and that the release recovers
after sustained limiting.

## Installation

### Windows
Copy `StinkyLimiter.clap` to:
```
%COMMONPROGRAMFILES%\CLAP\
```

### macOS
Copy `StinkyLimiter.clap` to:
```
/Library/Audio/Plug-Ins/CLAP/
```

### Linux
Copy `StinkyLimiter.clap` to:
```
~/.clap/
```

## Usage

1. **Broadcast delivery (EBU R128)**: threshold -1.0 dB, output -1.0 dB.
2. **ATSC A/85**: threshold -2.0 dB, output -2.0 dB.
3. **Safety limiter with headroom**: threshold -1.0 dB, output -6.0 dB.

### Tips
- Level and compress commentary and ambience separately upstream. The limiter
  is a peak stop, not a leveller; if it is working hard on the full mix, every
  crowd swell will duck the commentary with it.
- The latency is fixed for a given sample rate (about 5.1 ms). Hosts that read
  the CLAP latency extension will compensate automatically.
- The gain reduction meter reports the deepest reduction reached in the last
  block, in dB.

## How It Works

For every frame, both channels linked:

1. The input is written to a delay line and to a short history buffer.
2. A true-peak estimate is computed from the history using the BS.1770
   polyphase filter. The raw sample at the filter's alignment point is always
   included, which is what makes the sample-peak guarantee exact.
3. The target gain is `min(1, ceiling / peak)`.
4. Two envelopes follow the target: fast (instant attack, 80 ms release) and
   slow (200 ms attack, 500 ms release). The smaller of the two is used, so
   the result is never above the target.
5. A sliding minimum holds that gain for the length of the lookahead window.
6. A moving average over the same window turns the hold into a linear 5 ms
   ramp that reaches the required gain exactly when the peak arrives.
7. The gain is applied to the delayed audio, scaled to the output level, and
   hard-clipped at the output ceiling as a guard against floating-point
   rounding.

Because every averaged value at output time is a minimum over a window that
contains the detector frame for the sample being output, the applied gain is
always at or below the gain that sample required. That is the brickwall
guarantee, and it is exact for sample peaks.

### The ceiling contract

The true-peak ceiling is a bound, not a mathematical guarantee, for two
reasons. The gain changes from sample to sample, so interpolating the limited
output is not quite the same as limiting the interpolated input. And the
detector is the BS.1770 estimate, which is itself an approximation of the
continuous waveform. The first effect is second order for a smooth gain ramp,
and the second cancels against a meter that uses the same filter, so the
overshoot is small. Measured with an independent BS.1770 meter in the test
suite, it is under 0.0005 dB on tones, transients, square waves and sweeps at
every supported rate, and 0.003 dB on full-band white noise 10 dB over the
ceiling. The test pins it below 0.01 dB. A meter built on a different
interpolation filter can read a few hundredths of a dB differently on content
close to Nyquist.

Enforcing the ceiling in the oversampled domain would not turn the bound into
a guarantee: the downsampling filter reintroduces its own inter-sample
overshoot, and clipping there adds distortion the lookahead design avoids. The
bound plus a documented margin is the practical contract.

## Technical Details

- **Processing**: 32-bit floating point
- **Latency**: lookahead minus one sample plus the true-peak filter delay.
  245 samples at 48 kHz, 226 at 44.1 kHz, 485 at 96 kHz.
- **Lookahead**: 5 ms
- **Release**: 80 ms fast stage, 200 ms / 500 ms slow stage
- **True peak**: 4x oversampled detection up to 48 kHz, 2x at 88.2/96 kHz,
  sample peak at 176.4 kHz and above
- **Sample rates**: all standard rates supported
- **Algorithm**: feed-forward lookahead limiter with sliding-minimum hold and
  linear ramp

## License

MIT License - see [LICENSE](../LICENSE) file for details.

Copyright (c) 2025 Stinky Computing

## Support

- GitHub: https://github.com/stinkydev/audio-plugins
- Issues: Report bugs and request features on GitHub
