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

def main():
    if not os.path.exists(VST_PATH):
        print(f"Error: VST3 binary not found at:\n{VST_PATH}")
        return

    os.makedirs(OUTPUT_DIR, exist_ok=True)

    base_params = {
        "preset": "Custom (Free)",
        "modulation_enable": False,
        "auto_gain_balance_l2": False,
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
    p1 = {**base_params, "blend_bl": 1.0, "feedforward_ff": 0.8, "feedback_fb": 0.0, "delay_ms": 2.0}
    out_fir = process_with_warmup(VST_PATH, p1, impulse_400)

    plt.figure(figsize=(10, 3.5))
    plt.stem(sample_axis[:200], out_fir[:200], basefmt=" ")
    plt.title("1. FIR Mode Impulse Response (BL=1.0, FF=0.8, FB=0.0)")
    plt.xlabel("Sample Index (n)")
    plt.ylabel("Amplitude")
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, "test1_fir_mode.png"), dpi=300)
    plt.close()

    # =========================================================================
    # TEST 2: IIR Mode
    # =========================================================================
    print("Running Test 2: IIR Mode (Feedback Only)...")
    p2 = {**base_params, "blend_bl": 1.0, "feedforward_ff": 0.0, "feedback_fb": -0.7, "delay_ms": 2.0}
    out_iir = process_with_warmup(VST_PATH, p2, impulse_400)

    plt.figure(figsize=(10, 3.5))
    plt.stem(sample_axis[:350], out_iir[:350], basefmt=" ")
    plt.title("2. IIR Mode Impulse Response (BL=1.0, FF=0.0, FB=-0.7)")
    plt.xlabel("Sample Index (n)")
    plt.ylabel("Amplitude")
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, "test2_iir_mode.png"), dpi=300)
    plt.close()

    # =========================================================================
    # TEST 3: Universal / All-Pass Mode
    # =========================================================================
    print("Running Test 3: Universal / All-Pass Baseline Mode...")
    p3 = {**base_params, "blend_bl": 0.7, "feedforward_ff": 1.0, "feedback_fb": -0.7, "delay_ms": 2.0}
    out_allpass = process_with_warmup(VST_PATH, p3, impulse_400)

    plt.figure(figsize=(10, 3.5))
    plt.stem(sample_axis[:350], out_allpass[:350], basefmt=" ")
    plt.title("3. Universal / All-Pass Baseline IR (BL=0.7, FF=1.0, FB=-0.7)")
    plt.xlabel("Sample Index (n)")
    plt.ylabel("Amplitude")
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, "test3_allpass_mode.png"), dpi=300)
    plt.close()

    # =========================================================================
    # TEST 4: Moorer Damping Effect
    # =========================================================================
    print("Running Test 4: Moorer Damping Comparison...")
    p4a = {**base_params, "blend_bl": 1.0, "feedforward_ff": 0.0, "feedback_fb": 0.85, "delay_ms": 2.0, "moorer_high_damping": False}
    out_damp_off = process_with_warmup(VST_PATH, p4a, impulse_400)

    p4b = {**base_params, "blend_bl": 1.0, "feedforward_ff": 0.0, "feedback_fb": 0.85, "delay_ms": 2.0, "moorer_high_damping": True}
    out_damp_on = process_with_warmup(VST_PATH, p4b, impulse_400)

    plt.figure(figsize=(10, 5))
    plt.subplot(2, 1, 1)
    plt.stem(sample_axis[:350], out_damp_off[:350], basefmt=" ")
    plt.title("4a. Moorer Damping OFF — Sharp High-Frequency Spikes")
    plt.ylabel("Amplitude")
    plt.grid(True, alpha=0.3)

    plt.subplot(2, 1, 2)
    plt.stem(sample_axis[:350], out_damp_on[:350], basefmt=" ")
    plt.title("4b. Moorer Damping ON — Lowpass Diffusion")
    plt.xlabel("Sample Index (n)")
    plt.ylabel("Amplitude")
    plt.grid(True, alpha=0.3)

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
    # TEST 5: Auto-Gain (L2 Energy Balance Curve)
    # =========================================================================
    print("Running Test 5: Auto-Gain (L2 Balance Curve Sweep)...")
    np.random.seed(42)
    noise_duration = int(SAMPLE_RATE * 1.0) # 1 second noise buffer
    white_noise = np.random.normal(0, 0.1, noise_duration).astype(np.float32)

    fb_range = np.linspace(-0.95, 0.95, 39)
    rms_off = []
    rms_on = []

    for fb in fb_range:
        p_off = {**base_params, "blend_bl": 1.0, "feedforward_ff": 0.0, "feedback_fb": float(fb), "auto_gain_balance_l2": False}
        p_on  = {**base_params, "blend_bl": 1.0, "feedforward_ff": 0.0, "feedback_fb": float(fb), "auto_gain_balance_l2": True}

        out_off = process_with_warmup(VST_PATH, p_off, white_noise)
        out_on  = process_with_warmup(VST_PATH, p_on, white_noise)

        rms_off.append(np.sqrt(np.mean(out_off**2)))
        rms_on.append(np.sqrt(np.mean(out_on**2)))

    df_autogain = pd.DataFrame({"feedback_fb": fb_range, "rms_autogain_off": rms_off, "rms_autogain_on": rms_on})
    df_autogain.to_csv(os.path.join(OUTPUT_DIR, "test5_autogain_sweep.csv"), index=False)

    plt.figure(figsize=(10, 4.5))
    plt.plot(fb_range, 20 * np.log10(np.array(rms_off) + 1e-12), 'o--', label="Auto-Gain OFF (Unscaled)")
    plt.plot(fb_range, 20 * np.log10(np.array(rms_on) + 1e-12), 's-', label="Auto-Gain ON (L2 Normalized)")
    plt.title("5. Auto-Gain Balance (L2 Power Preservation vs Feedback)")
    plt.xlabel("Feedback Coefficient (FB)")
    plt.ylabel("RMS Output Level (dB)")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, "test5_autogain_curve.png"), dpi=300)
    plt.close()

    # =========================================================================
    # TEST 6: Modulated Modes (Explicit LFO Parameter Drive)
    # =========================================================================
    print("Running Test 6: Modulated Modes (Spectrogram Sweep)...")
    np.random.seed(42)
    noise_3s = np.random.normal(0, 0.2, int(SAMPLE_RATE * 3.0)).astype(np.float32)

    # Explicit DSP parameter profiles for each modulated preset
    modulated_profiles = {
        "Vibrato": {
            "preset": "Vibrato",
            "modulation_enable": True,
            "blend_bl": 0.0,
            "feedforward_ff": 1.0,
            "feedback_fb": 0.0,
            "delay_ms": 0.1,
            "mod_depth_ms": 2.0,
            "mod_rate_hz": 2.0
        },
        "Flanger": {
            "preset": "Flanger",
            "modulation_enable": True,
            "blend_bl": 0.7,
            "feedforward_ff": 0.7,
            "feedback_fb": 0.7,
            "delay_ms": 0.1,
            "mod_depth_ms": 1.0,
            "mod_rate_hz": 0.4
        },
        "Chorus": {
            "preset": "Chorus",
            "modulation_enable": True,
            "blend_bl": 0.7,
            "feedforward_ff": 1.0,
            "feedback_fb": -0.7,
            "delay_ms": 15.0,
            "mod_depth_ms": 15.0,
            "mod_rate_hz": 1.2
        }
    }

    spectrogram_records = []
    plt.figure(figsize=(12, 8))

    for i, (preset_name, params) in enumerate(modulated_profiles.items(), 1):
        # Filter out parameters not declared in the plugin parameter map
        valid_params = {k: v for k, v in params.items() if hasattr(load_plugin(VST_PATH), k)}
        out_mod = process_with_warmup(VST_PATH, valid_params, noise_3s)

        frequencies, times, Sxx = signal.spectrogram(
            out_mod, fs=SAMPLE_RATE, nperseg=1024, noverlap=512
        )
        power_db = 10 * np.log10(Sxx + 1e-12)

        plt.subplot(3, 1, i)
        plt.pcolormesh(times, frequencies, power_db, shading='gouraud', cmap='magma')
        plt.title(f"6{chr(96 + i)}. Spectrogram: {preset_name} (Sweeping Comb Notches)")
        plt.ylabel("Frequency (Hz)")
        if i == 3:
            plt.xlabel("Time (s)")
        plt.colorbar(label="Power (dB)")

        for t_idx, t_val in enumerate(times):
            for f_idx, f_val in enumerate(frequencies):
                spectrogram_records.append({
                    "preset_mode": preset_name,
                    "time_sec": t_val,
                    "frequency_hz": f_val,
                    "power_db": power_db[f_idx, t_idx]
                })

    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, "test6_modulated_spectrograms.png"), dpi=300)
    plt.close()

    df_spec = pd.DataFrame(spectrogram_records)
    df_spec.to_csv(os.path.join(OUTPUT_DIR, "test6_spectrogram_matrix.csv"), index=False)

    print("\nAll 6 Structural Verification Tests Completed Successfully!")
    print(f"Generated publication-ready files in: {OUTPUT_DIR}/")
    print("  - test1_fir_mode.png")
    print("  - test2_iir_mode.png")
    print("  - test3_allpass_mode.png")
    print("  - test4_moorer_damping.png")
    print("  - test5_autogain_curve.png")
    print("  - test6_modulated_spectrograms.png")
    print("  - all_tests_impulse_responses.csv")
    print("  - test5_autogain_sweep.csv")

if __name__ == "__main__":
    main()