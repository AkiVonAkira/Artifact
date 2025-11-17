#pragma once

#if defined(TRACY_ENABLE)
#   include <tracy/Tracy.hpp>
#   define ARTIFACT_FRAME_MARK FrameMark
#   define ARTIFACT_ZONE() ZoneScoped
#else
#   define ARTIFACT_FRAME_MARK ((void)0)
#   define ARTIFACT_ZONE() ((void)0)
#endif

namespace Artifact::Core {
struct FrameScopeGuard {
    FrameScopeGuard() { ARTIFACT_ZONE(); }
};
}
