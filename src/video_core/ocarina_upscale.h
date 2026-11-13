// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstdlib>
#include <string_view>

namespace VideoCommon {

enum class OcarinaUpscaleBackend { Native, Fsr, Dlss };

/// render_scaled: a DLSS render-scale patch shrank the scene to a size FSR 3.1 takes, so FSR 3.1
/// reconstructs it when DLSS can't run.
constexpr OcarinaUpscaleBackend SelectOcarinaUpscaler(bool fsr_enabled, bool dlss_enabled,
                                                    bool dlss_available, bool render_scaled,
                                                    std::string_view override) {
    if (override == "native") {
        return OcarinaUpscaleBackend::Native;
    }
    if (override == "fsr") {
        return OcarinaUpscaleBackend::Fsr;
    }
    if ((dlss_enabled || override == "dlss") && dlss_available) {
        return OcarinaUpscaleBackend::Dlss;
    }
    return fsr_enabled || render_scaled ? OcarinaUpscaleBackend::Fsr
                                        : OcarinaUpscaleBackend::Native;
}

/// EDEN_NX2_DLSS_JITTER enables DLSS jitter and LOD bias. Mode 1 uses the scene offset,
/// and mode 2 flips the vertical offset passed to Streamline.
inline int OcarinaDlssJitterMode() {
    static const int mode = [] {
        if (const char* value = std::getenv("EDEN_NX2_DLSS_JITTER")) {
            const std::string_view setting{value};
            if (setting == "1" || setting == "2") {
                return setting.front() - '0';
            }
        }
        return 0;
    }();
    return mode;
}

constexpr std::array<float, 2> OcarinaDlssJitterOffset(std::array<float, 2> offset, int mode) {
    if (mode == 2) {
        offset[1] = -offset[1];
    }
    return mode == 0 ? std::array<float, 2>{} : offset;
}

/// Texture LOD bias for docked OoT. FSR uses the 1440p-to-4K recommendation.
constexpr float OcarinaTextureLodBias(OcarinaUpscaleBackend backend, int dlss_jitter_mode) {
    if (backend == OcarinaUpscaleBackend::Fsr) {
        return -1.58f;
    }
    if (backend == OcarinaUpscaleBackend::Dlss && dlss_jitter_mode != 0) {
        return -1.75f;
    }
    return 0.0f;
}

/// Whether a sampler with that bias keeps it for an image: only docked (handheld isn't measured),
/// not for images the GPU wrote (render targets would read a mip too sharp), and not once the
/// upscaler failed this session (the game's own pass runs again).
constexpr bool KeepOcarinaTextureLodBias(bool docked, bool upscaler_failed, bool gpu_written) {
    return docked && !upscaler_failed && !gpu_written;
}

} // namespace VideoCommon
