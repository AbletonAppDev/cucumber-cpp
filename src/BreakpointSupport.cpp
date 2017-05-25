#include <cucumber-cpp/internal/step/BreakpointSupport.hpp>

namespace cucumber {
namespace internal {

static bool shouldBreakIntoDebugger = false;

CUCUMBER_CPP_EXPORT void shouldBreakIntoDebuggerInNextStep() {
    shouldBreakIntoDebugger = true;
}

CUCUMBER_CPP_EXPORT bool checkAndClearShouldBreakIntoDebugger() {
    const bool result = shouldBreakIntoDebugger;
    shouldBreakIntoDebugger = false;
    return result;
}

}
}
