#include "Artifact/Plugins/SplineShooterPlugin.h"

#include <spdlog/spdlog.h>

namespace Artifact::Games {

class SplineShooterLabModule final : public Plugins::SplineShooterPlugin {
public:
    void onRegister(const Plugins::PluginContext& ctx) override {
        Plugins::SplineShooterPlugin::onRegister(ctx);
        spdlog::info("SplineShooterLab gameplay module attached");
    }
};

}
