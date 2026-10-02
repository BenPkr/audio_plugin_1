#pragma once

#include <complex>
#include <vector>
#include <cmath>

namespace CombMath
{
    using Complex = std::complex<float>;

    /** Calculates transfer function H(e^(j*omega)) for a given normalized frequency omega (0 to pi rad/sample). */
    inline Complex evaluateTransferFunction(float omega, float delaySamples, float blend, float feedforward, float feedback)
    {
        // z^-M = e^(-j * omega * M) = cos(-omega * M) + j * sin(-omega * M)
        const float phase = -omega * delaySamples;
        const Complex z_inv_M = std::polar(1.0f, phase);

        const Complex num = blend + feedforward * z_inv_M;
        const Complex den = 1.0f - feedback * z_inv_M;

        // Avoid division by zero if feedback pushes pole onto unit circle
        if (std::abs(den) < 1e-6f)
            return Complex{ 0.0f, 0.0f };

        return num / den;
    }

    /** Calculates the M complex poles for H(z).
     *  Poles occur at z = (|FB|^(1/M)) * exp(j * (arg(FB) + 2k*pi) / M)
     */
    inline std::vector<Complex> calculatePoles(int delaySamples, float feedback)
    {
        std::vector<Complex> poles;
        if (delaySamples <= 0)
            return poles;

        poles.reserve(static_cast<size_t>(delaySamples));

        const float absFB = std::abs(feedback);
        // Radius of poles in the z-plane
        const float radius = (absFB > 1e-5f) ? std::pow(absFB, 1.0f / static_cast<float>(delaySamples)) : 0.0f;
        const float basePhase = (feedback < 0.0f) ? static_cast<float>(M_PI) : 0.0f;

        for (int k = 0; k < delaySamples; ++k)
        {
            const float angle = (basePhase + 2.0f * static_cast<float>(M_PI) * static_cast<float>(k)) / static_cast<float>(delaySamples);
            poles.push_back(std::polar(radius, angle));
        }

        return poles;
    }

    /** Calculates the M complex zeros for H(z).
     *  Zeros occur at z = (|-FF/BL|^(1/M)) * exp(j * (arg(-FF/BL) + 2k*pi) / M)
     */
    inline std::vector<Complex> calculateZeros(int delaySamples, float blend, float feedforward)
    {
        std::vector<Complex> zeros;
        if (delaySamples <= 0 || std::abs(blend) < 1e-6f)
            return zeros;

        zeros.reserve(static_cast<size_t>(delaySamples));

        const float ratio = -feedforward / blend;
        const float absRatio = std::abs(ratio);
        const float radius = (absRatio > 1e-5f) ? std::pow(absRatio, 1.0f / static_cast<float>(delaySamples)) : 0.0f;
        const float basePhase = (ratio < 0.0f) ? static_cast<float>(M_PI) : 0.0f;

        for (int k = 0; k < delaySamples; ++k)
        {
            const float angle = (basePhase + 2.0f * static_cast<float>(M_PI) * static_cast<float>(k)) / static_cast<float>(delaySamples);
            zeros.push_back(std::polar(radius, angle));
        }

        return zeros;
    }
}