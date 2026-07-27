#include "rgbpicker/backend_session.h"

#include <algorithm>
#include <utility>

namespace rgbpicker {
namespace {

constexpr std::chrono::milliseconds minimumSleep{1};

bool backendUnavailable(BackendError error)
{
    return error == BackendError::unavailable || error == BackendError::operationFailed;
}

}

std::chrono::milliseconds retryDelay(const BackendSessionConfig& config, int attempts)
{
    std::chrono::milliseconds delay{config.firstRetryDelay};
    for (int step{1}; step < attempts; ++step) {
        delay *= 2;
        if (delay >= config.maxRetryDelay) {
            return config.maxRetryDelay;
        }
    }
    return delay;
}

BackendSession::BackendSession(BackendFactory& factory, BackendSessionConfig config, SteadyNow now)
    : m_factory{factory}, m_config{config}, m_now{std::move(now)}
{
}

Backend* BackendSession::poll()
{
    if (m_backend != nullptr) {
        return m_backend.get();
    }
    if (m_retryScheduled && m_now() < m_nextAttempt) {
        return nullptr;
    }
    m_retryScheduled = false;

    if (m_config.mode == BackendMode::simulation) {
        m_backend = m_factory.createSimulation();
        if (m_backend == nullptr) {
            ++m_status.attempts;
            setPhase(BackendSessionPhase::waiting, "Simulation backend unavailable");
            scheduleRetry();
            return nullptr;
        }
        m_status.attempts = 0;
        setPhase(BackendSessionPhase::ready, "Simulation running");
        return m_backend.get();
    }
    return createBackend();
}

Backend* BackendSession::waitUntilReady(std::chrono::milliseconds budget, const Sleeper& sleep)
{
    const auto deadline{m_now() + budget};
    while (true) {
        if (Backend* const activeBackend{poll()}; activeBackend != nullptr) {
            return activeBackend;
        }
        const auto now{m_now()};
        if (now >= deadline) {
            return nullptr;
        }
        const auto untilRetry{m_retryScheduled
                                  ? std::chrono::duration_cast<std::chrono::milliseconds>(
                                        m_nextAttempt - now)
                                  : m_config.firstRetryDelay};
        const auto untilDeadline{
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now)};
        sleep(std::max(minimumSleep, std::min(untilRetry, untilDeadline)));
        if (m_now() <= now) {
            return nullptr;
        }
    }
}

void BackendSession::reportFailure(BackendError error)
{
    if (!backendUnavailable(error)) {
        return;
    }
    m_backend.reset();
    m_status.attempts = 0;
    setPhase(BackendSessionPhase::waiting, "Hardware unavailable, retrying");
    scheduleRetry();
}

void BackendSession::recreate()
{
    m_backend.reset();
    m_status.attempts = 0;
    setPhase(BackendSessionPhase::waiting, "Looking for newly attached hardware");
    scheduleRetry();
}

Backend* BackendSession::createBackend()
{
    ++m_status.attempts;
    auto created{m_factory.createHardware()};
    if (!created.has_value()) {
        setPhase(BackendSessionPhase::waiting, "No supported RGB hardware detected");
        scheduleRetry();
        return nullptr;
    }

    auto devices{(*created)->discover()};
    if (!devices.has_value() || devices->empty()) {
        setPhase(BackendSessionPhase::waiting, "Waiting for supported RGB hardware");
        scheduleRetry();
        return nullptr;
    }

    m_backend = std::move(*created);
    m_status.attempts = 0;
    setPhase(BackendSessionPhase::ready, "Hardware ready");
    return m_backend.get();
}

void BackendSession::scheduleRetry()
{
    m_nextAttempt = m_now() + retryDelay(m_config, m_status.attempts);
    m_retryScheduled = true;
}

void BackendSession::setPhase(BackendSessionPhase phase, std::string detail)
{
    m_status.phase = phase;
    m_status.detail = std::move(detail);
}

}
