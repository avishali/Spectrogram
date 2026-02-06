# Spectrogram

Hardware-accelerated spectrogram analyser plugin, built from the MelechDSP HQ template.

## Prerequisites

- CMake 3.22+
- C++20 compiler
- JUCE 8 (set `JUCE_PATH` or let CMake fetch via FetchContent)
- melechdsp-hq at `../melechdsp-hq` (sibling directory)

## Build

```bash
cd Sperctrogram
mkdir -p build && cd build
cmake .. -DJUCE_PATH=/path/to/JUCE  # or omit to fetch JUCE
cmake --build . -j
```

## Formats

- VST3
- AU (macOS)
- Standalone

## Features

- FFT Order: 256–16384 samples
- Overlap: 1–8x
- Window: Hann, Blackman-Harris, Flat Top
- Frequency scale: Linear / Logarithmic
- OpenGL-accelerated scrolling spectrogram
