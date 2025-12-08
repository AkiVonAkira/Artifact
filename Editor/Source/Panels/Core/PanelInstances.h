#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Panel.h"
#include "PanelRegistry.h"

namespace Artifact::Editor {
    struct PanelInstance {
        std::string instanceId;
        const PanelDescriptor *descriptor{nullptr};
        PanelPtr panel;
        bool open{true};
    };

    class PanelManager {
    public:
        PanelInstance *createInstance(const PanelDescriptor &descriptor);

        bool removeClosedInstances();

        std::vector<std::unique_ptr<PanelInstance> > &instances() { return instances_; }

    private:
        std::vector<std::unique_ptr<PanelInstance> > instances_;
        int nextCounter_{0};
    };
} // namespace Artifact::Editor
