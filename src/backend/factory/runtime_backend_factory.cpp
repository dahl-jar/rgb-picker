#include "rgbpicker/runtime_backend_factory.h"

#include "backend/hardware/hardware_backend.h"

namespace rgbpicker {

std::expected<std::unique_ptr<Backend>, BackendError> RuntimeBackendFactory::createHardware()
{
    auto backend{makeHardwareBackend()};
    if (backend == nullptr) {
        return std::unexpected{BackendError::unavailable};
    }
    return backend;
}

}
