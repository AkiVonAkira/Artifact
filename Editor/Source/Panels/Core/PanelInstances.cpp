#include "PanelInstances.h"

#include <algorithm>

namespace Artifact::Editor {
    PanelInstance *PanelManager::createInstance(const PanelDescriptor &descriptor) {
        auto instance = std::make_unique<PanelInstance>();
        instance->descriptor = &descriptor;
        instance->panel = descriptor.factory ? descriptor.factory() : nullptr;
        if (!instance->panel) {
            return nullptr;
        }
        instance->open = descriptor.closeable ? descriptor.defaultOpen : true;
        instance->instanceId = descriptor.id;
        if (descriptor.allowMultiple) {
            instance->instanceId += ":" + std::to_string(++nextCounter_);
        }
        auto *raw = instance.get();
        instances_.push_back(std::move(instance));
        return raw;
    }

    bool PanelManager::removeClosedInstances() {
        const auto sizeBefore = instances_.size();
        instances_.erase(std::remove_if(instances_.begin(), instances_.end(), [](const auto &instance) {
            return !instance || !instance->open;
        }), instances_.end());
        return instances_.size() != sizeBefore;
    }
}
