#include "rgbpicker/runtime_backend_factory.h"

#include "backend/hardware/hardware_backend.h"
#include "backend/simulation/simulation_backend.h"

namespace rgbpicker {

std::expected<std::unique_ptr<Backend>, BackendError> RuntimeBackendFactory::createHardware()
{
    auto backend{makeHardwareBackend()};
    if (backend == nullptr) {
        return std::unexpected{BackendError::unavailable};
    }
    return backend;
}

std::unique_ptr<Backend> RuntimeBackendFactory::createSimulation()
{
    return std::make_unique<SimulationBackend>();
}

}
