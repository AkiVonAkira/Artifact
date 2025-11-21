#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "Panel.h"

namespace Artifact::Editor {

struct PanelDescriptor {
    std::string id;
    std::string label;
    bool allowMultiple{true};
    bool defaultOpen{true};
    bool visibleInWindowMenu{true};
    bool hasWindow{true};
    bool closeable{true};
    std::function<PanelPtr()> factory;
};

const std::vector<PanelDescriptor>& GetPanelRegistry();
const PanelDescriptor* FindPanelDescriptor(std::string_view id);

}
