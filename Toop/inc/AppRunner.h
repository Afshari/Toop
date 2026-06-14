#pragma once
#include "Config.h"

#ifdef TOOP_DEBUG
#include "cpu/DebugRenderer.h"
#endif

namespace Toop {

    class AppRunner
    {
    public:
        static int Run(const Config& config);
        static int RunHeadless(const Config& config);
        static int RunBench(const Config& config);
    };

} // namespace Toop