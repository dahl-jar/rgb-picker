#pragma once

#include "rgbpicker/backend.h"

namespace rgbpicker {

class RuntimeBackendFactory final : public BackendFactory {
public:
    std::expected<std::unique_ptr<Backend>, BackendError>
    createHardware() override;
    std::unique_ptr<Backend> createSimulation() override;
};

}
