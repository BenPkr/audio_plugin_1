# Universal Comb Filter

Universal Comb Filter is a JUCE audio effect plugin with three selectable DSP
processors: a configurable comb filter, a ring modulator, and a tremolo. The
plugin is built as a VST3 effect and a JUCE Standalone application.

## Features and GUI

The editor has a **Processor Mode** selector for choosing the active processor.
Controls and visualizations adapt to the selected mode.

### Comb Filter

The Comb Filter mode includes five parameter presets:

- Custom (Free)
- Vibrato
- Flanger
- Chorus
- Doubling

The preset panels configure the comb filter's delay, blend, feedforward,
feedback, and modulation settings. The Auto Gain (L2) and Damp Tail controls
are available in this mode. A modulation plot displays the comb filter's LFO.

### Ring Modulator

Ring Mod applies a sine carrier to the input. Its mode controls are Ring Mix
and Carrier Frequency (Hz).

### Tremolo

Tremolo modulates the input amplitude. Its controls are Rate (Hz), Depth (%),
and Waveform (Sine, Triangle, or Square).

### Visual analysis

- The audio scope displays captured input and output waveforms. In Tremolo mode,
  it also displays the gain envelope.
- The frequency panel shows the comb response in Comb Filter mode and live
  input/output spectra in Ring Mod and Tremolo modes.
- The phase and pole-zero panels show comb-filter information in Comb Filter
  mode, these plots are inactive in Ring Mod and Tremolo modes.

## Build setup

### Requirements

- CMake 3.22 or newer
- A C++17-capable compiler
- On macOS, Xcode or the Xcode Command Line Tools
- Python 3 and pip to run the Pedalboard-based verification script

JUCE is included in `modules/JUCE`; a separate JUCE installation is not
required.

### Configure and build

From the repository root:

```sh
cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug --config Debug --parallel
```

CMake generates the VST3 and Standalone targets from `CMakeLists.txt`. The
project also adds JUCE's AudioPluginHost target to the build. Build artifacts
are placed under `cmake-build-debug/UniversalCombFilter_artefacts/`; with the
Debug configuration, the VST3 bundle is expected at:

```text
cmake-build-debug/UniversalCombFilter_artefacts/Debug/VST3/Universal Comb Filter.vst3
```

The Standalone application is placed under the corresponding `Standalone`
directory. Output locations can vary with the generator and build
configuration.

## Architecture and audio flow

The project separates the plugin shell, DSP modules, and editor visualizations:

- `source/PluginProcessor.*` owns the APVTS parameter state and DSP instances.
- `source/Parameters.h` defines the parameter IDs, ranges, and choices.
- `source/dsp/CombFilter.*`, `RingModulator.*`, and `Tremolo.*` implement the
  three processors. `Modulator.h` provides modulation for the Comb Filter.
- `source/PluginEditor.*` builds the GUI, attaches controls to APVTS parameters,
  and updates mode-dependent controls and visualizers.
- `source/gui/` contains the scope, response plots, modulation plot, and Comb
  preset panels.

At runtime, `processBlock()` reads the selected processor mode and routes each
audio block through that mode's DSP module. On a mode change, the newly selected
module is reset and the processor crossfades between the old and new processing
paths over approximately 10 ms. Input and output samples (and the Tremolo gain
envelope) are captured into fixed-capacity analysis buffers. The editor reads
those buffers on its timer; FFT work for the live spectrum is performed on the
GUI side rather than in the audio callback.

The plugin currently supports mono input and mono output.

Getting the plugin to work with stereo in audacity has been problematic and will be shelved

## Verification tests

`testing/run_vst_tests.py` is an audio integration/analysis harness. It loads
the built VST3 with Pedalboard, sets plugin parameters, processes generated
signals, and writes plots and CSV data into `testing/output/`.

The nine scenarios cover:

1. Comb Filter FIR impulse response
2. Comb Filter IIR impulse response
3. Comb Filter all-pass impulse response
4. Moorer damping comparison
5. Auto Gain (L2) feedback sweep
6. Flanger input/output spectrograms
7. Vibrato and Chorus output spectrograms
8. Tremolo input/output waveforms and expected gain envelope
9. Ring Mod input/output waveforms and FFT sidebands

### Install Python dependencies

From the repository root:

```sh
python3 -m venv testing/.venv
source testing/.venv/bin/activate
python -m pip install -r testing/requirements.txt
```

### Run the tests

Build the Debug VST3, install the dependencies/activate the venv, then run:

```sh
python testing/run_vst_tests.py
```

The script expects the VST3 bundle at the Debug path shown above. If using a
different build directory, configuration, or platform, update `VST_PATH` near
the top of `testing/run_vst_tests.py` to point to that build's VST3 bundle.

Generated PNG plots, CSV files, and other test artifacts are written to
`testing/output/`. This directory is ignored by Git.
