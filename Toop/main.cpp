#include "AppRunner.h"
#include "CLParser.h"
#include <iostream>

int main(int argc, char* argv[])
{
    auto parser = Toop::CLParser::Parse(argc, argv);
    auto config = Toop::CLParser::LoadConfig(parser.GetConfigDir());

    if (parser.IsHeadless())
        return Toop::AppRunner::RunHeadless(config);

    if (parser.IsBenchMode())
        return Toop::AppRunner::RunBench(config);

    return Toop::AppRunner::Run(config);
}