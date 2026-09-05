#pragma once

#include <cstdint>
#include <string>

namespace neta::core {

// Launch configuration parsed from the command line.
struct Config {
    std::uint64_t seed = 482913;  // default world seed
    bool headless = false;  // run the simulation without graphics
    int headlessTicks = 60;  // sim ticks to run in headless mode
    bool verbose = false;  // enable Debug-level logging
    bool showHelp = false;
    std::string loadPath;  // optional save file to load at startup

    static Config parse(int argc, char** argv);
    static std::string helpText();
};

}  // namespace neta::core
