#pragma once

#include <string>

namespace Artifact::Plugins {

struct PluginContext {
    std::string name;
};

class Plugin {
public:
    virtual ~Plugin() = default;
    virtual void onRegister(const PluginContext& ctx) = 0;
};

using PluginFactory = Plugin*(*)();

}
