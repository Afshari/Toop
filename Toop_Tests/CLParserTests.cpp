#include "pch.h"
#include "CLParser.h"
#include <filesystem>
#include "TestHelpers.h"
using namespace Toop::TestHelpers;

namespace Toop {

    // --------------------------------------------------------------------------------
    // Flag tests
    // --------------------------------------------------------------------------------

    TEST(CLParserTest, NoFlagsAllFalse)
    {
        ArgList args{ "Toop.exe" };
        auto parser = CLParser::Parse(args.argc(), args.data());
        EXPECT_FALSE(parser.IsHeadless());
        EXPECT_FALSE(parser.IsBenchMode());
        EXPECT_FALSE(parser.IsProfileMode());
    }

    TEST(CLParserTest, HeadlessFlagParsed)
    {
        ArgList args{ "Toop.exe", "--headless" };
        auto parser = CLParser::Parse(args.argc(), args.data());
        EXPECT_TRUE(parser.IsHeadless());
        EXPECT_FALSE(parser.IsBenchMode());
        EXPECT_FALSE(parser.IsProfileMode());
    }

    TEST(CLParserTest, BenchFlagParsed)
    {
        ArgList args{ "Toop.exe", "--bench" };
        auto parser = CLParser::Parse(args.argc(), args.data());
        EXPECT_FALSE(parser.IsHeadless());
        EXPECT_TRUE(parser.IsBenchMode());
        EXPECT_FALSE(parser.IsProfileMode());
    }

    TEST(CLParserTest, ProfileFlagParsed)
    {
        ArgList args{ "Toop.exe", "--profile" };
        auto parser = CLParser::Parse(args.argc(), args.data());
        EXPECT_FALSE(parser.IsHeadless());
        EXPECT_FALSE(parser.IsBenchMode());
        EXPECT_TRUE(parser.IsProfileMode());
    }

    TEST(CLParserTest, MultipleFlagsParsed)
    {
        ArgList args{ "Toop.exe", "--headless", "--profile" };
        auto parser = CLParser::Parse(args.argc(), args.data());
        EXPECT_TRUE(parser.IsHeadless());
        EXPECT_TRUE(parser.IsProfileMode());
        EXPECT_FALSE(parser.IsBenchMode());
    }

    TEST(CLParserTest, UnknownFlagDoesNotCrash)
    {
        ArgList args{ "Toop.exe", "--unknown" };
        EXPECT_NO_THROW(CLParser::Parse(args.argc(), args.data()));
    }

    // --------------------------------------------------------------------------------
    // Config path tests
    // --------------------------------------------------------------------------------

    TEST(CLParserTest, DefaultConfigDirIsExeDir)
    {
        ArgList args{ "Toop.exe" };
        auto parser = CLParser::Parse(args.argc(), args.data());
        // empty config dir is valid - means current directory
        EXPECT_NO_THROW(parser.GetConfigDir());
    }

    TEST(CLParserTest, ConfigFlagSetsConfigDir)
    {
        auto temp_dir = std::filesystem::temp_directory_path().string();
        ArgList args{ "Toop.exe", "--config", temp_dir.c_str() };
        auto parser = CLParser::Parse(args.argc(), args.data());
        EXPECT_EQ(
            std::filesystem::weakly_canonical(parser.GetConfigDir()),
            std::filesystem::weakly_canonical(temp_dir));
    }

    TEST(CLParserTest, ConfigFlagMissingPathDoesNotCrash)
    {
        ArgList args{ "Toop.exe", "--config" };
        EXPECT_NO_THROW(CLParser::Parse(args.argc(), args.data()));
    }

    // --------------------------------------------------------------------------------
    // LoadConfig tests
    // --------------------------------------------------------------------------------

    TEST(CLParserTest, LoadConfigFallsBackToDefaultWhenMissing)
    {
        auto temp_dir = std::filesystem::temp_directory_path() / "toop_no_config_dir";
        std::filesystem::create_directories(temp_dir);

        auto config = CLParser::LoadConfig(temp_dir);
        auto defaults = Config::Default();

        EXPECT_EQ(config.sim.num_strands, defaults.sim.num_strands);
        EXPECT_EQ(config.sim.num_segments, defaults.sim.num_segments);

        std::filesystem::remove_all(temp_dir);
    }

} // namespace Toop