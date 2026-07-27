#pragma once

#include "rgbpicker/backend.h"

#include <chrono>
#include <functional>
#include <memory>
#include <string>

namespace rgbpicker {

enum class BackendMode {
    hardware,
    simulation,
};

enum class BackendSessionPhase {
    idle,
    ready,
    waiting,
};

struct BackendSessionConfig {
    BackendMode mode{BackendMode::hardware};
    std::chrono::milliseconds firstRetryDelay{500};
    std::chrono::milliseconds maxRetryDelay{10000};
};

struct BackendSessionStatus {
    BackendSessionPhase phase{BackendSessionPhase::idle};
    std::string detail{"Hardware not ready"};
    int attempts{};
};

std::chrono::milliseconds retryDelay(const BackendSessionConfig& config, int attempts);

using SteadyNow = std::function<std::chrono::steady_clock::time_point()>;
using Sleeper = std::function<void(std::chrono::milliseconds)>;

class BackendSession {
public:
    BackendSession(BackendFactory& factory, BackendSessionConfig config, SteadyNow now);

    Backend* poll();
    Backend* waitUntilReady(std::chrono::milliseconds budget, const Sleeper& sleep);
    void reportFailure(BackendError error);
    void recreate();

    Backend* backend() const { return m_backend.get(); }
    BackendSessionStatus status() const { return m_status; }

private:
    Backend* createBackend();
    void scheduleRetry();
    void setPhase(BackendSessionPhase phase, std::string detail);

    BackendFactory& m_factory;
    BackendSessionConfig m_config;
    SteadyNow m_now;
    std::unique_ptr<Backend> m_backend;
    BackendSessionStatus m_status;
    std::chrono::steady_clock::time_point m_nextAttempt{};
    bool m_retryScheduled{false};
};

}
