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

    # TEST 1: FIR Mode
    p1 = {**base_params, "blend_bl": 1.0, "feedforward_ff": 0.8, "feedback_fb": 0.0, "delay_ms": 2.0}
    out_fir = process_with_warmup(VST_PATH, p1, num_samples)

    # TEST 2: IIR Mode
    p2 = {**base_params, "blend_bl": 1.0, "feedforward_ff": 0.0, "feedback_fb": -0.7, "delay_ms": 2.0}
    out_iir = process_with_warmup(VST_PATH, p2, num_samples)

    # TEST 3: All-Pass Mode
    p3 = {**base_params, "blend_bl": 0.7, "feedforward_ff": 1.0, "feedback_fb": -0.7, "delay_ms": 2.0}
    out_allpass = process_with_warmup(VST_PATH, p3, num_samples)

    # TEST 4a: Moorer Damping OFF
    p4a = {**base_params, "blend_bl": 1.0, "feedforward_ff": 0.0, "feedback_fb": 0.85, "delay_ms": 2.0, "moorer_high_damping": False}
    out_damp_off = process_with_warmup(VST_PATH, p4a, num_samples)

    # TEST 4b: Moorer Damping ON
    p4b = {**base_params, "blend_bl": 1.0, "feedforward_ff": 0.0, "feedback_fb": 0.85, "delay_ms": 2.0, "moorer_high_damping": True}
    out_damp_on = process_with_warmup(VST_PATH, p4b, num_samples)

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