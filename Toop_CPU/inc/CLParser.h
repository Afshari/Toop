#pragma once
#include "Config.h"
#include <filesystem>

namespace Toop {

    class CLParser
    {
    public:
        static CLParser Parse(int argc, char* argv[]);
        static Config   LoadConfig(const std::filesystem::path& configDir);

        bool IsHeadless()     const { return m_headless; }
        bool IsBenchMode()    const { return m_benchMode; }
        bool IsProfileMode()  const { return m_profileMode; }

        const std::filesystem::path& GetConfigDir() const { return m_configDir; }

    private:
        bool                  m_headless = false;
        bool                  m_benchMode = false;
        bool                  m_profileMode = false;
        std::filesystem::path m_configDir;
    };

} // namespace Toop