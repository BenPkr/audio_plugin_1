import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from pedalboard import load_plugin

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
VST_PATH = os.path.join(
    SCRIPT_DIR,
    "../cmake-build-debug/UniversalCombFilter_artefacts/Debug/VST3/Universal Comb Filter.vst3"
)
OUTPUT_DIR = os.path.join(SCRIPT_DIR, "output")
SAMPLE_RATE = 44100.0

def process_with_warmup(plugin_path, params, num_samples=500):
    """
    Applies parameters, warms up the plugin, then feeds a unit impulse in the
    same host call so parameter smoothing can settle before the impulse.
    """
    plugin = load_plugin(plugin_path)

    for param_name, val in params.items():
        if param_name not in plugin.parameters:
            raise ValueError(f"Plugin parameter not found: {param_name}")
        setattr(plugin, param_name, val)

    warmup_samples = 1024
    input_buffer = np.zeros((1, warmup_samples + num_samples), dtype=np.float32)
    input_buffer[0, warmup_samples] = 1.0

    output = plugin(input_buffer, sample_rate=SAMPLE_RATE)[0]
    return output[warmup_samples:]

def main():
    if not os.path.exists(VST_PATH):
        print(f"Error: VST3 binary not found at:\n{VST_PATH}")
        return

    os.makedirs(OUTPUT_DIR, exist_ok=True)

    # Base dictionary for Custom (Free) mode
    base_params = {
        "preset": "Custom (Free)",
        "modulation_enable": False,
        "auto_gain_balance_l2": False,
        "moorer_high_damping": False,
    }

    num_samples = 400
    sample_axis = np.arange(num_samples)

    # =========================================================================
    # TEST 1: FIR Mode
    # =========================================================================
    print("Running Test 1: FIR Mode (Feedforward Only)...")
    p1 = {**base_params, "blend_bl": 1.0, "feedforward_ff": 0.8, "feedback_fb": 0.0, "delay_ms": 2.0}
    out_fir = process_with_warmup(VST_PATH, p1, num_samples)

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
    out_iir = process_with_warmup(VST_PATH, p2, num_samples)

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
    out_allpass = process_with_warmup(VST_PATH, p3, num_samples)

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
    out_damp_off = process_with_warmup(VST_PATH, p4a, num_samples)

    p4b = {**base_params, "blend_bl": 1.0, "feedforward_ff": 0.0, "feedback_fb": 0.85, "delay_ms": 2.0, "moorer_high_damping": True}
    out_damp_on = process_with_warmup(VST_PATH, p4b, num_samples)

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

    # =========================================================================
    # Export Master CSV
    # =========================================================================
    print("Exporting consolidated dataset to CSV...")
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

    print("\nAll tests completed successfully!")
    print(f"Generated PNG plots & CSV dataset in: {OUTPUT_DIR}/")

if __name__ == "__main__":
    main()