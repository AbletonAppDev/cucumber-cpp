#ifndef CUKE_BREAKPOINTSUPPORT_HPP_
#define CUKE_BREAKPOINTSUPPORT_HPP_

#include "CukeExport.hpp"

#include <signal.h>

namespace cucumber {
namespace internal {

#if defined(_MSC_VER)
#define CUKE_BREAK_INTO_DEBUGGER __debugbreak()

#elif defined(__GNUC__) && (__i386 || __x86_64)
#define CUKE_BREAK_INTO_DEBUGGER asm("int3")

#else // Generic Unix
#define CUKE_BREAK_INTO_DEBUGGER raise(SIGTRAP)

#endif // COMPILER CHECK


CUCUMBER_CPP_EXPORT void shouldBreakIntoDebuggerInNextStep();
CUCUMBER_CPP_EXPORT bool checkAndClearShouldBreakIntoDebugger();

}
}

#endif /* CUKE_BREAKPOINTSUPPORT_HPP_ */
