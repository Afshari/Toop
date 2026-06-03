#include "CLParser.h"
#include <iostream>

namespace Toop {

    // --------------------------------------------------------------------------------
    CLParser CLParser::Parse(int argc, char* argv[])
    {
        CLParser parser;

        parser.m_configDir = std::filesystem::weakly_canonical(
            std::filesystem::path(argv[0]).parent_path()
        );

        for (int i = 1; i < argc; i++)
        {
            std::string arg = argv[i];
            if (arg == "--headless")
                parser.m_headless = true;
            else if (arg == "--bench")
                parser.m_benchMode = true;
            else if (arg == "--profile")
                parser.m_profileMode = true;
            else if (arg == "--config" && i + 1 < argc)
                parser.m_configDir = std::filesystem::weakly_canonical(
                    std::filesystem::path(argv[++i])
                );
            else
                std::cerr << "[WARN] Unknown argument: " << arg << std::endl;
        }

        return parser;
    }

    // --------------------------------------------------------------------------------
    Config CLParser::LoadConfig(const std::filesystem::path& configDir)
    {
        auto configPath = configDir / "config.json";
        if (!std::filesystem::exists(configPath))
        {
            std::cout << "[WARN] config.json not found, using defaults." << std::endl;
            return Config::Default();
        }

        auto config = Config::Load(configPath.string());

        if (std::filesystem::path(config.profile.output_dir).is_relative())
            config.profile.output_dir = std::filesystem::weakly_canonical(
                configDir / config.profile.output_dir).string();

        return config;
    }

} // namespace Toop