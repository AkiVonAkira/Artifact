#pragma once

#include "Artifact/Plugins/PluginAPI.h"

namespace Artifact::Plugins {

class SplineShooterPlugin final : public Plugin {
public:
    void onRegister(const PluginContext& ctx) override;
};

}
