#pragma once

#include <algorithm>
#include <cmath>

namespace PhaserMath
{
    struct AllPassCoefficients
    {
        double c;
        double d;
    };

    inline AllPassCoefficients getAllPassCoefficients(double centerFrequencyHz,
                                                      double sampleRate)
    {
        constexpr double pi = 3.14159265358979323846;

        // The fixed-Q design defines Q = fc/fb, so fb = fc/Q.
        constexpr double q = 0.7071067811865476;

        // Zolzer page 55 equation 2.28: omega is term inside cos
        // omega = 2*pi*fc/fs.
        const double omega = 2.0 * pi * centerFrequencyHz / sampleRate;

        // Keep the bandwidth below Nyquist so equation 2.27 yields stable coefficients.
        const double bandwidthHz = std::min(centerFrequencyHz / q, sampleRate * 0.45);
        const double normalizedBandwidth = pi * bandwidthHz / sampleRate;
        const double tangentBandwidth = std::tan(normalizedBandwidth);

        // Zolzer page 55 equation 2.27: c = (tan(pi*fb/fs)-1)/(tan(pi*fb/fs)+1).
        const double c = (tangentBandwidth - 1.0) / (tangentBandwidth + 1.0);

        // Zolzer page 55 equation 2.28: d = -cos(2*pi*fc/fs).
        const double d = -std::cos(omega);

        return { c, d };
    }
}
