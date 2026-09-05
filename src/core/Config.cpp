#include "core/Config.h"

namespace neta::core {

Config Config::parse(int argc, char** argv) {
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto needValue = [&](const std::string& flag) -> std::string {
            if (i + 1 >= argc) {
                cfg.showHelp = true;
                return {};
            }
            ++i;
            (void)flag;
            return argv[i];
        };
        if (arg == "--seed") {
            const std::string value = needValue(arg);
            try {
                cfg.seed = std::stoull(value);
            } catch (...) {
                cfg.showHelp = true;
            }
        } else if (arg == "--headless") {
            cfg.headless = true;
        } else if (arg == "--ticks") {
            const std::string value = needValue(arg);
            try {
                cfg.headlessTicks = std::stoi(value);
                if (cfg.headlessTicks < 0) {
                    cfg.headlessTicks = 0;
                }
            } catch (...) {
                cfg.showHelp = true;
            }
        } else if (arg == "--load") {
            cfg.loadPath = needValue(arg);
        } else if (arg == "--verbose") {
            cfg.verbose = true;
        } else if (arg == "--help" || arg == "-h") {
            cfg.showHelp = true;
        } else {
            cfg.showHelp = true;
        }
    }
    return cfg;
}

std::string Config::helpText() {
    return "Usage: neta [options]\n"
           "\n"
           "Options:\n"
           "  --seed N     World seed (default 482913). Same seed = same world.\n"
           "  --headless   Run the simulation without opening a window.\n"
           "  --ticks N    Headless sim ticks to run (default 60).\n"
           "  --load PATH  Load a save file at startup.\n"
           "  --verbose    Enable Debug-level logging.\n"
           "  --help, -h   Show this text.\n"
           "\n"
           "In-game keys:\n"
           "  WASD / arrows  Move    Esc  Quit    Space  Pause sim\n"
           "  E / Z / Enter  Talk, pick up, advance text\n"
           "  F1  Debug overlay      F2  Scanlines  F5  Quicksave  F9  Quickload\n";
}

}  // namespace neta::core
