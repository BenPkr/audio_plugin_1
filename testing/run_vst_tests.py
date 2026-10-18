import os
import numpy as np
import pandas as pd
import scipy.signal as signal
import matplotlib.pyplot as plt
from pedalboard import load_plugin

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
VST_PATH = os.path.join(
    SCRIPT_DIR,
    "../cmake-build-debug/UniversalCombFilter_artefacts/Debug/VST3/Universal Comb Filter.vst3"
)
OUTPUT_DIR = os.path.join(SCRIPT_DIR, "output")
SAMPLE_RATE = 44100.0

def process_with_warmup(plugin_path, params, input_signal, warmup_samples=1024):
    """
    Applies parameters, warms up the plugin with silence, then feeds input_signal
    in a single contiguous host call to ensure state stability.
    """
    plugin = load_plugin(plugin_path)

    for param_name, val in params.items():
        if param_name not in plugin.parameters:
            raise ValueError(f"Plugin parameter not found: {param_name}")
        setattr(plugin, param_name, val)

    num_samples = len(input_signal)
    input_buffer = np.zeros((1, warmup_samples + num_samples), dtype=np.float32)
    input_buffer[0, warmup_samples:] = input_signal

    output = plugin(input_buffer, sample_rate=SAMPLE_RATE)[0]
    return output[warmup_samples:]

def format_time_domain_plot(ax, delay_ms=2.0, fs=SAMPLE_RATE):
    """
    Adds a secondary top X-axis showing time in milliseconds alongside sample index (n).
    """
    ax.set_xlabel("Sample Index (n)")
    ax.grid(True, alpha=0.3)
    
    # Create secondary top axis for Time (ms)
    ax_top = ax.secondary_xaxis('top', functions=(lambda n: n / (fs / 1000.0), lambda ms: ms * (fs / 1000.0)))
    ax_top.set_xlabel("Time (ms)")
    
    # Primary reflection delay mark (e.g., tau = 2.0 ms -> n = 88.2)
    delay_samples = delay_ms * (fs / 1000.0)
    ax.axvline(x=delay_samples, color='red', linestyle='--', alpha=0.5, label=f"Delay τ = {delay_ms} ms ({delay_samples:.1f} samples)")
    ax.legend(loc="upper right")

def main():
    if not os.path.exists(VST_PATH):
        print(f"Error: VST3 binary not found at:\n{VST_PATH}")
        return

    os.makedirs(OUTPUT_DIR, exist_ok=True)

    base_params = {
        "processing_mode": "Comb Filter",
        "preset": "Custom (Free)",
        "modulation_enable": False,
        "auto_gain_balance_l_infinity": False,
        "moorer_high_damping": False,
    }

    # Signal generators
    impulse_400 = np.zeros(400, dtype=np.float32)
    impulse_400[0] = 1.0
    sample_axis = np.arange(400)

    # =========================================================================
    # TEST 1: FIR Mode
    # =========================================================================
    print("Running Test 1: FIR Mode (Feedforward Only)...")
    p1 = {**base_params, "blend_ring_mix": 1.0, "feedforward_ff": 0.8, "feedback_fb": 0.0, "delay_ms": 2.0}
    out_fir = process_with_warmup(VST_PATH, p1, impulse_400)

    fig, ax = plt.subplots(figsize=(10, 3.8))
    ax.stem(sample_axis[:200], out_fir[:200], basefmt=" ")
    ax.set_title("1. FIR Mode Impulse Response (BL=1.0, FF=0.8, FB=0.0, Delay=2.0 ms / 88.2 samples)")
    ax.set_ylabel("Amplitude")
    format_time_domain_plot(ax, delay_ms=2.0)
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, "test1_fir_mode.png"), dpi=300)
    plt.close()

    # =========================================================================
    # TEST 2: IIR Mode
    # =========================================================================
    print("Running Test 2: IIR Mode (Feedback Only)...")
    p2 = {**base_params, "blend_ring_mix": 1.0, "feedforward_ff": 0.0, "feedback_fb": -0.7, "delay_ms": 2.0}
    out_iir = process_with_warmup(VST_PATH, p2, impulse_400)

    fig, ax = plt.subplots(figsize=(10, 3.8))
    ax.stem(sample_axis[:350], out_iir[:350], basefmt=" ")
    ax.set_title("2. IIR Mode Impulse Response (BL=1.0, FF=0.0, FB=-0.7, Delay=2.0 ms / 88.2 samples)")
    ax.set_ylabel("Amplitude")
    format_time_domain_plot(ax, delay_ms=2.0)
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, "test2_iir_mode.png"), dpi=300)
    plt.close()

    # =========================================================================
    # TEST 3: Universal / All-Pass Mode
    # =========================================================================
    print("Running Test 3: Universal / All-Pass Baseline Mode...")
    p3 = {**base_params, "blend_ring_mix": 0.7, "feedforward_ff": 1.0, "feedback_fb": -0.7, "delay_ms": 2.0}
    out_allpass = process_with_warmup(VST_PATH, p3, impulse_400)

    fig, ax = plt.subplots(figsize=(10, 3.8))
    ax.stem(sample_axis[:350], out_allpass[:350], basefmt=" ")
    ax.set_title("3. Universal / All-Pass Baseline IR (BL=0.7, FF=1.0, FB=-0.7, Delay=2.0 ms / 88.2 samples)")
    ax.set_ylabel("Amplitude")
    format_time_domain_plot(ax, delay_ms=2.0)
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, "test3_allpass_mode.png"), dpi=300)
    plt.close()

    # =========================================================================
    # TEST 4: Moorer Damping Effect
    # =========================================================================
    print("Running Test 4: Moorer Damping Comparison...")
    p4a = {**base_params, "blend_ring_mix": 1.0, "feedforward_ff": 0.0, "feedback_fb": 0.85, "delay_ms": 2.0, "moorer_high_damping": False}
    out_damp_off = process_with_warmup(VST_PATH, p4a, impulse_400)

    p4b = {**base_params, "blend_ring_mix": 1.0, "feedforward_ff": 0.0, "feedback_fb": 0.85, "delay_ms": 2.0, "moorer_high_damping": True}
    out_damp_on = process_with_warmup(VST_PATH, p4b, impulse_400)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 5.5))
    ax1.stem(sample_axis[:350], out_damp_off[:350], basefmt=" ")
    ax1.set_title("4a. Moorer Damping OFF — Sharp High-Frequency Spikes (Delay=2.0 ms / 88.2 samples)")
    ax1.set_ylabel("Amplitude")
    format_time_domain_plot(ax1, delay_ms=2.0)

    ax2.stem(sample_axis[:350], out_damp_on[:350], basefmt=" ")
    ax2.set_title("4b. Moorer Damping ON — Lowpass Diffusion (Delay=2.0 ms / 88.2 samples)")
    ax2.set_ylabel("Amplitude")
    format_time_domain_plot(ax2, delay_ms=2.0)

    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, "test4_moorer_damping.png"), dpi=300)
    plt.close()

    # Export IR Dataset to Master CSV
    print("Exporting impulse response dataset to CSV...")
    df_results = pd.DataFrame({
        "sample_index": sample_axis,
        "fir_mode": out_fir,
        "iir_mode": out_iir,
        "allpass_mode": out_allpass,
        "moorer_damping_off": out_damp_off,
        "moorer_damping_on": out_damp_on
    })
    master_csv_path = os.path.join(OUTPUT_DIR, "all_tests_impulse_responses.csv")
    df_results.to_csv(master_csv_path, index=False)

    # =========================================================================
    # TEST 5: Auto-Gain (L-infinity Gain Balance Curve)
    # =========================================================================
    print("Running Test 5: Auto-Gain (L-infinity Balance Curve Sweep)...")
    np.random.seed(42)
    noise_duration = int(SAMPLE_RATE * 1.0)
    white_noise = np.random.normal(0, 0.1, noise_duration).astype(np.float32)

    fb_range = np.linspace(-0.95, 0.95, 39)
    rms_off = []
    rms_on = []

    for fb in fb_range:
        p_off = {**base_params, "blend_ring_mix": 1.0, "feedforward_ff": 0.0, "feedback_fb": float(fb), "auto_gain_balance_l_infinity": False}
        p_on  = {**base_params, "blend_ring_mix": 1.0, "feedforward_ff": 0.0, "feedback_fb": float(fb), "auto_gain_balance_l_infinity": True}

        out_off = process_with_warmup(VST_PATH, p_off, white_noise)
        out_on  = process_with_warmup(VST_PATH, p_on, white_noise)

        rms_off.append(np.sqrt(np.mean(out_off**2)))
        rms_on.append(np.sqrt(np.mean(out_on**2)))

    df_autogain = pd.DataFrame({"feedback_fb": fb_range, "rms_autogain_off": rms_off, "rms_autogain_on": rms_on})
    df_autogain.to_csv(os.path.join(OUTPUT_DIR, "test5_autogain_sweep.csv"), index=False)

    plt.figure(figsize=(10, 5))
    plt.plot(fb_range, 20 * np.log10(np.array(rms_off) + 1e-12), 'o--', label="Auto-Gain OFF (Unscaled)")
    plt.plot(fb_range, 20 * np.log10(np.array(rms_on) + 1e-12), 's-', label="Auto-Gain ON (L-infinity Normalized: g = 1 - |FB|)")
    plt.axvline(x=0.0, color='gray', linestyle=':', alpha=0.7, label="FB = 0 (Unity Gain g = 1.0)")
    
    # Annotate L-infinity formula and unity gain point.
    plt.annotate(
        "Unity Gain at FB = 0.0\ng(0) = 1 - |0| = 1.0",
        xy=(0.0, 20 * np.log10(rms_on[len(rms_on)//2] + 1e-12)),
        xytext=(-0.45, -15),
        arrowprops=dict(facecolor='black', shrink=0.05, width=1, headwidth=5),
        fontsize=9,
        bbox=dict(boxstyle="round,pad=0.3", fc="yellow", alpha=0.4)
    )
    
    plt.title("5. Auto-Gain Balance (L-infinity Gain Normalization vs Feedback)", fontsize=11, fontweight='bold', pad=12)
    plt.xlabel("Feedback Coefficient (FB)")
    plt.ylabel("RMS Output Level (dB)")
    plt.grid(True, alpha=0.3)
    plt.legend(loc="upper right", frameon=True, facecolor='white', framealpha=0.9)
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, "test5_autogain_curve.png"), dpi=300)
    plt.close()

    # =========================================================================
    # TEST 6: Flanger Mode (Input White Noise vs. Output Spectrogram)
    # =========================================================================
    print("Running Test 6: Flanger Mode (Input Noise vs Output Spectrogram)...")
    np.random.seed(42)
    noise_3s = np.random.normal(0, 0.2, int(SAMPLE_RATE * 3.0)).astype(np.float32)

    flanger_params = {
        "preset": "Flanger",
        "modulation_enable": True,
        "blend_ring_mix": 0.7,
        "feedforward_ff": 0.7,
        "feedback_fb": 0.7,
        "delay_ms": 1.5,
        "modulation_depth": 1.0,
        "lfo_carrier_frequency_hz": 0.4
    }
    out_flanger = process_with_warmup(VST_PATH, flanger_params, noise_3s)

    f_in, t_in, S_in = signal.spectrogram(noise_3s, fs=SAMPLE_RATE, nperseg=1024, noverlap=512)
    f_out, t_out, S_out = signal.spectrogram(out_flanger, fs=SAMPLE_RATE, nperseg=1024, noverlap=512)

    plt.figure(figsize=(12, 7))

    plt.subplot(2, 1, 1)
    plt.pcolormesh(t_in, f_in, 10 * np.log10(S_in + 1e-12), shading='gouraud', cmap='magma')
    plt.title("6a. Input Signal: Flat Gaussian White Noise Floor")
    plt.ylabel("Frequency (Hz)")
    plt.colorbar(label="Power (dB)")

    plt.subplot(2, 1, 2)
    plt.pcolormesh(t_out, f_out, 10 * np.log10(S_out + 1e-12), shading='gouraud', cmap='magma')
    plt.title("6b. Flanger Output: Sweeping Resonant Comb Notches")
    plt.ylabel("Frequency (Hz)")
    plt.xlabel("Time (s)")
    plt.colorbar(label="Power (dB)")

    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, "test6_flanger_spectrogram.png"), dpi=300)
    plt.close()

    # Export Dataset
    spectrogram_records = []
    for t_idx, t_val in enumerate(t_out):
        for f_idx, f_val in enumerate(f_out):
            spectrogram_records.append({
                "time_sec": t_val,
                "frequency_hz": f_val,
                "input_power_db": 10 * np.log10(S_in[f_idx, t_idx] + 1e-12),
                "flanger_power_db": 10 * np.log10(S_out[f_idx, t_idx] + 1e-12)
            })
    pd.DataFrame(spectrogram_records).to_csv(os.path.join(OUTPUT_DIR, "test6_flanger_spectrogram_matrix.csv"), index=False)

    # =========================================================================
    # TEST 7: Vibrato & Chorus (Input Harmonic vs. Output Spectrograms)
    # =========================================================================
    print("Running Test 7: Vibrato & Chorus (Input vs Output Spectrograms 0–2kHz)...")
    t_3s = np.linspace(0, 3.0, int(SAMPLE_RATE * 3.0), endpoint=False)
    harmonic_signal = (0.2 * np.sin(2 * np.pi * 440.0 * t_3s) + 0.1 * np.sin(2 * np.pi * 880.0 * t_3s)).astype(np.float32)

    target_profiles = {
        "Vibrato": {
            "preset": "Vibrato",
            "modulation_enable": True,
            "blend_ring_mix": 0.0,
            "feedforward_ff": 1.0,
            "feedback_fb": 0.0,
            "delay_ms": 0.1,
            "modulation_depth": 3.0,
            "lfo_carrier_frequency_hz": 5.0
        },
        "Chorus": {
            "preset": "Chorus",
            "modulation_enable": True,
            "blend_ring_mix": 0.7,
            "feedforward_ff": 1.0,
            "feedback_fb": 0.0,
            "delay_ms": 20.0,
            "modulation_depth": 30.0,
            "lfo_carrier_frequency_hz": 1.2
        }
    }

    # Compute Input Signal STFT
    f_harm, t_harm, Z_in = signal.stft(harmonic_signal, fs=SAMPLE_RATE, nperseg=2048, noverlap=1024)
    power_in_db = 10 * np.log10(np.abs(Z_in)**2 + 1e-12)

    plt.figure(figsize=(12, 10))

    # Row 1: Input Reference Signal
    plt.subplot(3, 1, 1)
    plt.pcolormesh(t_harm, f_harm, power_in_db, shading='gouraud', cmap='magma')
    plt.ylim([0, 2000])
    plt.title("7a. Input Reference Signal: Stationary Harmonics (440 Hz & 880 Hz)")
    plt.ylabel("Frequency (Hz)")
    plt.colorbar(label="Power (dB)")

    phase_records = []

    for idx, (profile_name, params) in enumerate(target_profiles.items(), 2):
        out_target = process_with_warmup(VST_PATH, params, harmonic_signal)

        f_out, t_out, Z_out = signal.stft(out_target, fs=SAMPLE_RATE, nperseg=2048, noverlap=1024)
        power_out_db = 10 * np.log10(np.abs(Z_out)**2 + 1e-12)

        plt.subplot(3, 1, idx)
        plt.pcolormesh(t_out, f_out, power_out_db, shading='gouraud', cmap='magma')
        plt.ylim([0, 2000])
        plt.title(f"7{chr(96 + idx)}. {profile_name} Output: Pitch Motion Analysis (0–2 kHz)")
        plt.ylabel("Frequency (Hz)")
        if idx == 3:
            plt.xlabel("Time (s)")
        plt.colorbar(label="Power (dB)")

        for t_i, t_v in enumerate(t_out):
            for f_i, f_v in enumerate(f_out):
                if f_v <= 2000:
                    phase_records.append({
                        "preset_mode": profile_name,
                        "time_sec": t_v,
                        "frequency_hz": f_v,
                        "power_db": power_out_db[f_i, t_i]
                    })

    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, "test7_vibrato_chorus_zoomed_spectrograms.png"), dpi=300)
    plt.close()

    pd.DataFrame(phase_records).to_csv(os.path.join(OUTPUT_DIR, "test7_phase_analysis_matrix.csv"), index=False)

    # =========================================================================
    # TEST 8: Tremolo Envelope
    # =========================================================================
    print("Running Test 8: Tremolo Envelope Verification...")
    tremolo_duration = 3.0
    tremolo_time = np.linspace(0.0, tremolo_duration, int(SAMPLE_RATE * tremolo_duration), endpoint=False)
    tremolo_input_frequency = 110.0
    tremolo_input_amplitude = 0.2
    tremolo_rate = 5.0
    tremolo_depth = 0.8
    tremolo_input = (
        tremolo_input_amplitude * np.sin(2.0 * np.pi * tremolo_input_frequency * tremolo_time)
    ).astype(np.float32)
    tremolo_output = process_with_warmup(
        VST_PATH,
        {
            "processing_mode": "Tremolo",
            "tremolo_rate_hz": tremolo_rate,
            "modulation_depth": 80.0,
            "tremolo_waveform": "Sine",
        },
        tremolo_input,
    )
    visible_duration = 0.5
    visible_samples = int(SAMPLE_RATE * visible_duration)
    visible_time = tremolo_time[:visible_samples]
    phase = ((np.arange(visible_samples) + 1024) * tremolo_rate / SAMPLE_RATE) % 1.0
    expected_lfo = 0.5 * (np.sin(2.0 * np.pi * phase) + 1.0)
    expected_gain = 1.0 - tremolo_depth * (1.0 - expected_lfo)
    expected_envelope = tremolo_input_amplitude * expected_gain

    fig, (input_ax, output_ax) = plt.subplots(2, 1, figsize=(12, 7), sharex=True)
    input_ax.plot(
        visible_time,
        tremolo_input[:visible_samples],
        color="tab:orange",
        alpha=0.8,
        label="Input",
    )
    input_ax.set_title("Input")
    input_ax.set_ylabel("Amplitude")
    input_ax.grid(True, alpha=0.3)
    input_ax.legend(loc="upper right")

    output_ax.plot(
        visible_time,
        tremolo_output[:visible_samples],
        color="tab:blue",
        linewidth=1.0,
        label="Tremolo output",
    )
    output_ax.plot(
        visible_time,
        expected_envelope,
        "k--",
        linewidth=1.0,
        label="Expected output envelope",
    )
    output_ax.plot(visible_time, -expected_envelope, "k--", linewidth=1.0)
    output_ax.set_title("Tremolo output with expected gain envelope")
    output_ax.set_xlabel("Time (s)")
    output_ax.set_ylabel("Amplitude")
    output_ax.grid(True, alpha=0.3)
    output_ax.legend(loc="upper right")
    fig.suptitle("8a. Tremolo Waveforms (110 Hz Input, 5 Hz LFO, 80% Depth)")
    fig.tight_layout()
    fig.savefig(os.path.join(OUTPUT_DIR, "test8_tremolo_time_domain.png"), dpi=300)
    plt.close()

    # =========================================================================
    # TEST 9: Ring Modulator Beat Pattern and Sidebands
    # =========================================================================
    print("Running Test 9: Ring Modulator Beat Pattern and Sideband Verification...")
    ring_input_frequency = 10.0
    ring_carrier_frequency = 440.0
    ring_duration = 3.0
    ring_time = np.linspace(0.0, ring_duration, int(SAMPLE_RATE * ring_duration), endpoint=False)
    ring_input_amplitude = 0.2
    ring_input = (
        ring_input_amplitude * np.sin(2.0 * np.pi * ring_input_frequency * ring_time)
    ).astype(np.float32)
    ring_params = {
        "processing_mode": "Ring Modulator",
        "blend_ring_mix": 1.0,
        "lfo_carrier_frequency_hz": ring_carrier_frequency,
    }
    ring_output = process_with_warmup(VST_PATH, ring_params, ring_input)

    ring_visible_duration = 0.4
    ring_visible_samples = int(SAMPLE_RATE * ring_visible_duration)
    ring_visible_time = ring_time[:ring_visible_samples]
    ring_beat_nulls = np.arange(
        0.0, ring_visible_duration, 1.0 / (2.0 * ring_input_frequency)
    )

    fig, (input_ax, output_ax) = plt.subplots(2, 1, figsize=(12, 7), sharex=True)
    input_ax.plot(
        ring_visible_time,
        ring_input[:ring_visible_samples],
        color="tab:orange",
        alpha=0.75,
        label=f"Input ({ring_input_frequency:.0f} Hz)",
    )
    input_ax.set_title("Input")
    input_ax.set_ylabel("Amplitude")
    input_ax.grid(True, alpha=0.3)
    input_ax.legend(loc="upper right")

    output_ax.plot(
        ring_visible_time,
        ring_output[:ring_visible_samples],
        color="tab:blue",
        linewidth=1.0,
        label="Ring Mod output",
    )
    for index, beat_null in enumerate(ring_beat_nulls):
        output_ax.axvline(
            beat_null,
            color="tab:red",
            linestyle="--",
            alpha=0.45,
            label="Expected beat-envelope nulls" if index == 0 else None,
        )
    output_ax.set_title("Ring Mod output with expected beat-envelope nulls")
    output_ax.set_xlabel("Time (s)")
    output_ax.set_ylabel("Amplitude")
    output_ax.grid(True, alpha=0.3)
    output_ax.legend(loc="upper right")
    fig.suptitle(
        f"9a. Ring Mod Waveforms ({ring_input_frequency:.0f} Hz Input × "
        f"{ring_carrier_frequency:.0f} Hz Carrier)"
    )
    fig.tight_layout()
    fig.savefig(os.path.join(OUTPUT_DIR, "test9_ringmod_time_domain.png"), dpi=300)
    plt.close()

    ring_frequencies_in, ring_power_in = signal.periodogram(
        ring_input, fs=SAMPLE_RATE, nfft=len(ring_input)
    )
    ring_frequencies_out, ring_power_out = signal.periodogram(
        ring_output, fs=SAMPLE_RATE, nfft=len(ring_output)
    )
    ring_difference_frequency = abs(ring_carrier_frequency - ring_input_frequency)
    ring_sum_frequency = ring_carrier_frequency + ring_input_frequency

    fig, (input_ax, output_ax) = plt.subplots(2, 1, figsize=(12, 8), sharex=True)
    input_band = ring_frequencies_in <= 1500.0
    output_band = ring_frequencies_out <= 1500.0
    input_ax.plot(
        ring_frequencies_in[input_band],
        10.0 * np.log10(ring_power_in[input_band] + 1e-15),
        color="tab:orange",
        label=f"Input FFT ({ring_input_frequency:.0f} Hz tone)",
    )
    input_ax.axvline(
        ring_input_frequency,
        color="tab:red",
        linestyle="--",
        alpha=0.75,
        label=f"Input tone: {ring_input_frequency:.0f} Hz",
    )
    input_ax.set_title(f"Input spectrum: {ring_input_frequency:.0f} Hz tone")
    input_ax.set_ylabel("Power (dB)")
    input_ax.grid(True, alpha=0.3)
    input_ax.legend(loc="upper right")

    output_ax.plot(
        ring_frequencies_out[output_band],
        10.0 * np.log10(ring_power_out[output_band] + 1e-15),
        color="tab:blue",
        label="Ring Mod output FFT",
    )
    for index, (frequency, label) in enumerate((
        (ring_difference_frequency, f"Expected difference: {ring_difference_frequency:.0f} Hz"),
        (ring_sum_frequency, f"Expected sum: {ring_sum_frequency:.0f} Hz"),
    )):
        output_ax.axvline(
            frequency,
            color="tab:red",
            linestyle="--",
            alpha=0.75,
            label=label,
        )
    output_ax.set_title(
        f"Ring Mod output spectrum with expected sidebands "
        f"at {ring_difference_frequency:.0f} Hz and {ring_sum_frequency:.0f} Hz"
    )
    output_ax.set_xlim(0.0, 1500.0)
    output_ax.set_xlabel("Frequency (Hz)")
    output_ax.set_ylabel("Power (dB)")
    output_ax.grid(True, alpha=0.3)
    output_ax.legend(loc="upper right")
    fig.suptitle(
        f"9b. Ring Mod Input and Output FFT "
        f"({ring_input_frequency:.0f} Hz Input × {ring_carrier_frequency:.0f} Hz Carrier)"
    )
    fig.tight_layout()
    fig.savefig(os.path.join(OUTPUT_DIR, "test9_ringmod_fft.png"), dpi=300)
    plt.close()

    print("\nAll 9 Verification Tests Completed Successfully!")
    print(f"Generated artifacts in: {OUTPUT_DIR}/")

if __name__ == "__main__":
    main()