#include "Artifact/Plugins/SplineShooterPlugin.h"

#include <spdlog/spdlog.h>

namespace Artifact::Plugins {

void SplineShooterPlugin::onRegister(const PluginContext& ctx) {
    spdlog::info("SplineShooterPlugin registered for project {}", ctx.name);
}

}
