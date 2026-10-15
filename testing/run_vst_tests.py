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

def process_with_warmup(plugin, params, num_samples=500):
    """
    Applies parameters, processes a warm-up buffer to flush parameter 
    smoothing / latency, then feeds a unit impulse.
    """
    # 1. Apply parameters
    for param_name, val in params.items():
        if hasattr(plugin, param_name):
            setattr(plugin, param_name, val)

    # 2. Process warm-up silence to settle parameter smoothing (1024 samples)
    warmup_buffer = np.zeros((1, 1024), dtype=np.float32)
    plugin(warmup_buffer, sample_rate=SAMPLE_RATE)

    # 3. Create unit impulse
    impulse_input = np.zeros((1, num_samples), dtype=np.float32)
    impulse_input[0, 0] = 1.0

    # 4. Capture actual impulse response
    output = plugin(impulse_input, sample_rate=SAMPLE_RATE)[0]
    return output

def main():
    if not os.path.exists(VST_PATH):
        print(f"Error: VST3 binary not found at:\n{VST_PATH}")
        return

    os.makedirs(OUTPUT_DIR, exist_ok=True)
    plugin = load_plugin(VST_PATH)

    # Base dictionary for Custom (Free) mode
    base_params = {
        "preset": "Custom (Free)",
        "modulation_enable": False,
        "auto_gain_balance_l2": False,
        "moorer_high_damping": False,
    }

    num_samples = 400
    sample_axis = np.arange(num_samples)

    # TEST 1: FIR Mode
    p1 = {**base_params, "blend_bl": 1.0, "feedforward_ff": 0.8, "feedback_fb": 0.0, "delay_ms": 2.0}
    out_fir = process_with_warmup(plugin, p1, num_samples)

    # TEST 2: IIR Mode
    p2 = {**base_params, "blend_bl": 1.0, "feedforward_ff": 0.0, "feedback_fb": -0.7, "delay_ms": 2.0}
    out_iir = process_with_warmup(plugin, p2, num_samples)

    # TEST 3: All-Pass Mode
    p3 = {**base_params, "blend_bl": 0.7, "feedforward_ff": 1.0, "feedback_fb": -0.7, "delay_ms": 2.0}
    out_allpass = process_with_warmup(plugin, p3, num_samples)

    # TEST 4a: Moorer Damping OFF
    p4a = {**base_params, "blend_bl": 1.0, "feedforward_ff": 0.0, "feedback_fb": 0.85, "delay_ms": 2.0, "moorer_high_damping": False}
    out_damp_off = process_with_warmup(plugin, p4a, num_samples)

    # TEST 4b: Moorer Damping ON
    p4b = {**base_params, "blend_bl": 1.0, "feedforward_ff": 0.0, "feedback_fb": 0.85, "delay_ms": 2.0, "moorer_high_damping": True}
    out_damp_on = process_with_warmup(plugin, p4b, num_samples)

    # Export CSV
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
    print(f"Updated CSV saved to {master_csv_path}")

if __name__ == "__main__":
    main()