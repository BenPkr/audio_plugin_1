# Universal Comb Filter

This JUCE project builds three independent audio effect plugins: Universal Comb
Filter, a multi-mode effect with a configurable comb filter, ring modulator,
tremolo, and phaser; Feedforward Compressor, a stereo compressor/limiter; and
Downward Expander. Each is built as a VST3 effect and a JUCE Standalone
application.

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
feedback, and modulation settings. The Auto Gain (L-infinity) and Damp Tail controls
are available in this mode. A modulation plot displays the comb filter's LFO.

### Ring Modulator

Ring Mod applies a sine carrier to the input. Its mode controls are Ring Mix
and Carrier Frequency (Hz).

### Tremolo

Tremolo modulates the input amplitude. Its controls are Rate (Hz), Depth (%),
and Waveform (Sine, Triangle, or Square).

### Phaser

The Phaser cascades 2-8 second-order all-pass sections and blends their output
with the dry signal. Its controls set the number of sections (notches), LFO rate
(0.05-8 Hz), sweep depth (up to a two-octave movement), and wet mix. The LFO
plot shows sweep position, while the frequency response displays the live
response and the range traversed by the notches.

### Visual analysis

- The audio scope aligns input and output waveforms to the nearest rising input
  threshold crossing, with a free-running fallback when no crossing is found.
  In Tremolo mode, it also displays the correspondingly aligned gain envelope.
- The frequency panel shows the comb response in Comb Filter mode and live
  input/output spectra in Ring Mod and Tremolo modes. Phaser mode shows its
  current response and the response envelope over the LFO sweep.

- The phase and pole-zero panels show comb-filter information in Comb Filter
  mode; these plots are inactive in Ring Mod, Tremolo, and Phaser modes.

## Feedforward Compressor

The compressor/limiter is a separate plugin target with its own processor,
parameter state, and editor. It supports matched mono and stereo layouts and
provides Threshold, Ratio, Knee, Attack, Release, and Make-Up Gain controls.
Its transfer-curve display shows the current threshold/knee response, make-up
gain, average sidechain input level, and average gain reduction for the most
recent block.

The detector averages the left and right channels for a stereo sidechain,
computes the soft-knee gain reduction in dB, and applies separate attack and
release ballistics to that gain-reduction envelope. The resulting gain is
applied equally to each channel. Ratio ranges from 1:1 to 19.9:1, with the
control's endpoint selecting an infinite-ratio limiter. Meter readouts are
rounded to whole dB and have no additional temporal smoothing.

## Downward Expander

Downward Expander is an independent plugin target with its own processor and
editor. It has four controls: Threshold, Ratio, Attack, and Release. Below the
threshold, attenuation increases according to the selected expansion ratio;
there is no range control or maximum attenuation setting. Like the compressor,
it supports matched mono and stereo layouts and applies linked gain equally to
both channels. The editor includes a live input/output transfer curve, a marker
for the current block-average input level, and whole-dB readouts of average
input level and attenuation for the most recent block.

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

CMake generates VST3 and Standalone targets for all three plugins from
`CMakeLists.txt`. The project also adds JUCE's AudioPluginHost target to the
build. Build artifacts are placed under each plugin's artefact directory. With
the Debug configuration, the VST3 bundles are expected at:

```text
cmake-build-debug/UniversalCombFilter_artefacts/Debug/VST3/Universal Comb Filter.vst3
cmake-build-debug/FeedforwardCompressor_artefacts/Debug/VST3/Feedforward Compressor.vst3
cmake-build-debug/DownwardExpander_artefacts/Debug/VST3/Downward Expander.vst3
```

The Standalone applications are placed under the corresponding `Standalone`
directories. Output locations can vary with the generator and build
configuration.

## Architecture and audio flow

The project separates the plugin shell, DSP modules, and editor visualizations:

- `source/PluginProcessor.*` owns the APVTS parameter state and DSP instances.
- `source/Parameters.h` defines the parameter IDs, ranges, and choices.
- `source/dsp/CombFilter.*`, `RingModulator.*`, `Tremolo.*`, and `Phaser.*`
  implement the four processors. `Modulator.h` provides modulation for the
  Comb Filter.
- `source/PluginEditor.*` builds the GUI, attaches controls to APVTS parameters,
  and updates mode-dependent controls and visualizers.
- `source/gui/` contains the scope, response plots, modulation plot, and Comb
  preset panels.
- `source/compressor/` contains the independent compressor plugin's processor,
  parameter layout, editor, DSP, and transfer-curve visualization.
- `source/expander/` contains the independent expander plugin's processor,
  parameter layout, editor, and DSP.

At runtime, `processBlock()` reads the selected processor mode and routes each
audio block through that mode's DSP module. On a mode change, the newly selected
module is reset and the processor crossfades between the old and new processing
paths over approximately 10 ms. Input and output samples (and the Tremolo gain
envelope) are captured into fixed-capacity analysis buffers. The editor reads
those buffers on its timer; FFT work for the live spectrum is performed on the
GUI side rather than in the audio callback.

Universal Comb Filter currently supports mono input and mono output.
Feedforward Compressor supports matched mono or stereo input/output layouts.
Downward Expander supports matched mono or stereo input/output layouts.

Getting Universal Comb Filter to work with stereo in Audacity has been
problematic and is shelved for now.

## Verification tests

`testing/run_vst_tests.py` is an audio integration/analysis harness for
Universal Comb Filter. It loads that plugin's built VST3 with Pedalboard, sets
plugin parameters, processes generated signals, and writes plots and CSV data
into `testing/output/`. It does not yet test Feedforward Compressor or
Downward Expander.

The nine scenarios cover:

1. Comb Filter FIR impulse response
2. Comb Filter IIR impulse response
3. Comb Filter all-pass impulse response
4. Moorer damping comparison
5. Auto Gain (L-infinity) feedback sweep
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
