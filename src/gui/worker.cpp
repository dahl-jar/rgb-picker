#include "gui/worker.h"

namespace rgbpicker::gui {

void Worker::postBrightness(int brightness)
{
    post([this, brightness] {
        m_brightness = brightness;
        std::vector<rgbpicker::DeviceColor> current;
        {
            const std::lock_guard lock{m_state.mutex};
            current = m_state.applied;
        }
        applyEntries(current);
    });
}

void Worker::postZoneColor(std::uint32_t deviceId, std::uint32_t zoneId, Color color)
{
    post([this, deviceId, zoneId, color] {
        rememberColor(color);
        ensureColorableMode(deviceId);
        runDeviceOp([&](Backend& backend) {
            return backend.changeZoneColor(deviceId, zoneId,
                                           rgbpicker::scaleBrightness(color, m_brightness));
        });
    });
}

void Worker::postResize(std::uint32_t deviceId, std::uint32_t zoneId, int size)
{
    post([this, deviceId, zoneId, size] {
        runDeviceOp([&](Backend& backend) {
            return backend.resizeZone(deviceId, zoneId, size);
        });
        rememberZoneSize(deviceId, zoneId, size);
    });
}

void Worker::postMode(std::uint32_t deviceId, std::string mode)
{
    post([this, deviceId, mode = std::move(mode)] {
        runDeviceOp([&](Backend& backend) {
            return backend.changeMode(deviceId, mode);
        });
    });
}

void Worker::postAllDevicesColor(Color color)
{
    post([this, color] {
        rememberColor(color);
        colorEveryDevice(color);
    });
}

int Worker::pending()
{
    const std::lock_guard lock{m_state.mutex};
    return m_state.pendingJobs;
}

void Worker::post(std::function<void()> job)
{
    {
        const std::lock_guard lock{m_mutex};
        m_jobs.push_back(std::move(job));
    }
    {
        const std::lock_guard lock{m_state.mutex};
        ++m_state.pendingJobs;
    }
    m_wake.notify_one();
}

void Worker::run(const std::stop_token& stopToken)
{
    while (!stopToken.stop_requested()) {
        std::function<void()> job;
        {
            std::unique_lock lock{m_mutex};
            m_wake.wait_for(lock, stopToken, maintenanceInterval,
                            [this] { return !m_jobs.empty(); });
            if (stopToken.stop_requested()) {
                return;
            }
            if (!m_jobs.empty()) {
                job = std::move(m_jobs.front());
                m_jobs.pop_front();
            }
        }
        if (job == nullptr) {
            maintainBackend();
            flushLook();
            continue;
        }
        job();
        const std::lock_guard lock{m_state.mutex};
        --m_state.pendingJobs;
    }
}

void Worker::maintainBackend()
{
    const bool backendReady{m_session.poll() != nullptr};
    if (backendReady && !m_backendReady) {
        m_backendReady = true;
        m_lastProbe = std::chrono::steady_clock::now();
        refreshOnWorker();
        beginRestore();
        return;
    }
    if (backendReady) {
        continueRestore();
        if (m_backendReady &&
            std::chrono::steady_clock::now() - m_lastProbe >= backendProbeInterval) {
            m_lastProbe = std::chrono::steady_clock::now();
            probeBackend();
            return;
        }
        publishStatus();
        return;
    }
    m_backendReady = backendReady;
    publishStatus();
}

void Worker::probeBackend()
{
    Backend* const backend{m_session.backend()};
    if (backend == nullptr) {
        return;
    }
    auto devices{backend->discover()};
    if (!devices.has_value()) {
        noteError(devices.error());
        return;
    }
    const std::lock_guard lock{m_state.mutex};
    if (devices->size() != m_state.devices.size()) {
        m_state.devices = std::move(*devices);
        ++m_state.revision;
    }
}

void Worker::refreshOnWorker()
{
    Backend* const backend{m_session.backend()};
    if (backend == nullptr) {
        publishStatus();
        return;
    }
    auto devices{backend->discover()};
    if (!devices.has_value()) {
        noteError(devices.error());
        return;
    }
    {
        const std::lock_guard lock{m_state.mutex};
        m_state.devices = std::move(*devices);
        ++m_state.revision;
    }
    publishStatus();
}

void Worker::restoreLayout()
{
    Backend* const backend{m_session.backend()};
    if (backend == nullptr || m_layout.empty()) {
        return;
    }
    for (const rgbpicker::ZoneSize& remembered : m_layout) {
        const auto deviceId{findDeviceId(remembered.device)};
        if (!deviceId.has_value()) {
            continue;
        }
        const auto zoneId{findZoneId(*deviceId, remembered.zone)};
        if (zoneId.has_value()) {
            static_cast<void>(backend->resizeZone(*deviceId, *zoneId, remembered.size));
        }
    }
    refreshOnWorker();
}

void Worker::rememberZoneSize(std::uint32_t deviceId, std::uint32_t zoneId, int size)
{
    if (m_layoutStore == nullptr) {
        return;
    }
    std::string device;
    std::string zone;
    {
        const std::lock_guard lock{m_state.mutex};
        const auto found{std::ranges::find(m_state.devices, deviceId, &Device::id)};
        if (found == m_state.devices.end() || zoneId >= found->zones.size()) {
            return;
        }
        device = found->name;
        zone = found->zones[zoneId].name;
    }
    rgbpicker::storeZoneSize(m_layout, rgbpicker::ZoneSize{std::move(device), std::move(zone),
                                                            size});
    m_layoutStore->save(m_layout);
}

std::vector<rgbpicker::DeviceColor> Worker::restoreLastLook()
{
    if (!m_restoreColor) {
        return {};
    }
    std::vector<rgbpicker::DeviceColor> entries;
    Color color{};
    {
        const std::lock_guard lock{m_state.mutex};
        entries = m_state.applied;
        color = m_state.lastColor;
    }
    if (entries.empty()) {
        colorEveryDevice(color);
        return {};
    }
    return applyEntries(entries);
}

void Worker::beginRestore()
{
    if (m_restoreStarted) {
        continueRestore();
        return;
    }
    m_restoreStarted = true;
    const auto now{std::chrono::steady_clock::now()};
    m_restoreDeadline = now + restoreWindow;
    m_nextRestoreAttempt = now + m_restoreRetry;
    restoreLayout();
    m_pendingRestore = restoreLastLook();
}

void Worker::continueRestore()
{
    if (m_pendingRestore.empty()) {
        return;
    }
    const auto now{std::chrono::steady_clock::now()};
    if (now >= m_restoreDeadline) {
        m_pendingRestore.clear();
        return;
    }
    if (now < m_nextRestoreAttempt) {
        return;
    }
    m_restoreRetry *= 2;
    m_nextRestoreAttempt = now + m_restoreRetry;

    const std::size_t waiting{m_pendingRestore.size()};
    m_pendingRestore = applyEntries(m_pendingRestore);
    if (m_pendingRestore.size() == waiting) {
        m_session.recreate();
        m_backendReady = false;
    }
}

void Worker::colorEveryDevice(Color color)
{
    std::vector<std::uint32_t> ids;
    {
        const std::lock_guard lock{m_state.mutex};
        ids.reserve(m_state.devices.size());
        for (const Device& device : m_state.devices) {
            ids.push_back(device.id);
        }
    }
    for (const std::uint32_t id : ids) {
        if (!writeDeviceColor(id, color)) {
            return;
        }
    }
}

void Worker::flushLook()
{
    if (m_appliedStore == nullptr) {
        return;
    }
    std::vector<rgbpicker::DeviceColor> current;
    {
        const std::lock_guard lock{m_state.mutex};
        current = m_state.applied;
    }
    current = rgbpicker::sortedByDevice(std::move(current));
    if (current == m_savedLook) {
        return;
    }
    m_savedLook = std::move(current);
    m_appliedStore->save(m_savedLook);
}

std::vector<rgbpicker::DeviceColor>
Worker::applyEntries(const std::vector<rgbpicker::DeviceColor>& entries)
{
    std::vector<rgbpicker::DeviceColor> unwritten;
    for (const rgbpicker::DeviceColor& entry : entries) {
        const auto deviceId{findDeviceId(entry.device)};
        if (!deviceId.has_value() || !writeDeviceColor(*deviceId, entry.color)) {
            unwritten.push_back(entry);
        }
    }
    return unwritten;
}

void Worker::ensureColorableMode(std::uint32_t deviceId)
{
    Backend* const backend{m_session.backend()};
    if (backend == nullptr) {
        return;
    }
    std::vector<rgbpicker::Mode> modes;
    std::string activeMode;
    {
        const std::lock_guard lock{m_state.mutex};
        const auto device{std::ranges::find(m_state.devices, deviceId, &Device::id)};
        if (device == m_state.devices.end()) {
            return;
        }
        modes = device->modes;
        activeMode = device->activeMode;
    }
    const auto target{rgbpicker::chooseDirectMode(modes, activeMode)};
    if (!target.has_value()) {
        return;
    }
    auto device{backend->changeMode(deviceId, *target)};
    if (!device.has_value()) {
        noteError(device.error());
        return;
    }
    storeDevice(std::move(*device));
}

bool Worker::writeDeviceColor(std::uint32_t deviceId, Color color)
{
    Backend* const backend{m_session.backend()};
    if (backend == nullptr) {
        return false;
    }
    ensureColorableMode(deviceId);
    auto device{backend->changeDeviceColor(deviceId, rgbpicker::scaleBrightness(color,
                                                                            m_brightness))};
    if (!device.has_value()) {
        noteError(device.error());
        return false;
    }
    rememberColor(color);
    rememberDeviceColor(deviceId, color);
    storeDevice(std::move(*device));
    return true;
}

void Worker::rememberColor(Color color)
{
    const std::lock_guard lock{m_state.mutex};
    m_state.lastColor = color;
}

void Worker::rememberDeviceColor(std::uint32_t deviceId, Color color)
{
    const std::lock_guard lock{m_state.mutex};
    const auto device{std::ranges::find(m_state.devices, deviceId, &Device::id)};
    if (device == m_state.devices.end()) {
        return;
    }
    const auto entry{
        std::ranges::find(m_state.applied, device->name, &rgbpicker::DeviceColor::device)};
    if (entry == m_state.applied.end()) {
        m_state.applied.push_back(rgbpicker::DeviceColor{device->name, color});
        return;
    }
    entry->color = color;
}

std::optional<std::uint32_t> Worker::findDeviceId(const std::string& name)
{
    const std::lock_guard lock{m_state.mutex};
    const auto device{std::ranges::find(m_state.devices, name, &Device::name)};
    if (device == m_state.devices.end()) {
        return std::nullopt;
    }
    return device->id;
}

std::optional<std::uint32_t> Worker::findZoneId(std::uint32_t deviceId, const std::string& name)
{
    const std::lock_guard lock{m_state.mutex};
    const auto device{std::ranges::find(m_state.devices, deviceId, &Device::id)};
    if (device == m_state.devices.end()) {
        return std::nullopt;
    }
    const auto zone{std::ranges::find(device->zones, name, &Zone::name)};
    return zone == device->zones.end() ? std::nullopt : std::optional{zone->id};
}

void Worker::runDeviceOp(const std::function<std::expected<Device, BackendError>(Backend&)>& op)
{
    Backend* const backend{m_session.backend()};
    if (backend == nullptr) {
        return;
    }
    auto device{op(*backend)};
    if (!device.has_value()) {
        noteError(device.error());
        return;
    }
    storeDevice(std::move(*device));
}

void Worker::noteError(BackendError error)
{
    m_session.reportFailure(error);
    m_backendReady = m_session.backend() != nullptr;
    setStatus(std::format("Error: {}", errorText(error)));
}

void Worker::storeDevice(Device device)
{
    const std::lock_guard lock{m_state.mutex};
    const auto existing{std::ranges::find(m_state.devices, device.id, &Device::id)};
    if (existing != m_state.devices.end()) {
        *existing = std::move(device);
    }
    ++m_state.revision;
}

void Worker::setStatus(std::string text)
{
    const std::lock_guard lock{m_state.mutex};
    m_state.status = std::move(text);
}

void Worker::publishStatus()
{
    const rgbpicker::BackendSessionStatus status{m_session.status()};
    if (status.phase == m_publishedPhase && status.detail == m_publishedDetail) {
        return;
    }
    m_publishedPhase = status.phase;
    m_publishedDetail = status.detail;
    const std::lock_guard lock{m_state.mutex};
    m_state.backendReady = status.phase == BackendSessionPhase::ready;
    m_state.status = status.detail;
}

}
