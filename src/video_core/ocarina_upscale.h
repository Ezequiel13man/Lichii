// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <cstdlib>
#include <string_view>

namespace VideoCommon {

enum class OcarinaUpscaleBackend { Native, Fsr, Dlss };

constexpr OcarinaUpscaleBackend SelectOcarinaUpscaler(bool fsr_enabled, bool dlss_enabled,
                                                    bool dlss_available,
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
    return fsr_enabled ? OcarinaUpscaleBackend::Fsr : OcarinaUpscaleBackend::Native;
}

/// The UI requests changes; the GPU thread applies them between frames so the scene, DLSS
/// constants and sampler variants use the same mode. Mode 2 flips Streamline's vertical offset.
struct OcarinaDlssJitterControl {
    const int initial_mode = [] {
        const char* value = std::getenv("EDEN_NX2_DLSS_JITTER");
        if (value != nullptr && (value[0] == '0' || value[0] == '1')) {
            return value[0] - '0';
        }
        return 2;
    }();
    std::atomic<int> requested{initial_mode};
    std::atomic<int> applied{initial_mode};

    bool Toggle() {
        const int enabled_mode = initial_mode == 1 ? 1 : 2;
        const int next = requested.load(std::memory_order_relaxed) == 0 ? enabled_mode : 0;
        requested.store(next, std::memory_order_relaxed);
        return next != 0;
    }

    bool Apply() {
        const int next = requested.load(std::memory_order_relaxed);
        return applied.exchange(next, std::memory_order_relaxed) != next;
    }
};

inline OcarinaDlssJitterControl& OcarinaDlssJitter() {
    static OcarinaDlssJitterControl control;
    return control;
}

inline int OcarinaDlssJitterMode() {
    return OcarinaDlssJitter().applied.load(std::memory_order_relaxed);
}

/// At docked 1440p to 4K, FSR adds log2(render / output) - 1 to mipmapped samplers.
/// DLSS adds log2(render / output) when its texture bias is enabled.
constexpr float OcarinaFsrLodBias(OcarinaUpscaleBackend backend, bool dlss_bias_enabled = false) {
    if (backend == OcarinaUpscaleBackend::Fsr) {
        return -1.58f;
    }
    return backend == OcarinaUpscaleBackend::Dlss && dlss_bias_enabled ? -0.58f : 0.0f;
}

/// Whether a sampler with that bias keeps it for an image: only docked (handheld isn't measured),
/// not for images the GPU wrote (render targets would read a mip too sharp), and not once the
/// upscaler failed this session (the game's own pass runs again).
constexpr bool KeepOcarinaFsrLodBias(bool docked, bool upscaler_failed, bool gpu_written) {
    return docked && !upscaler_failed && !gpu_written;
}

} // namespace VideoCommon
