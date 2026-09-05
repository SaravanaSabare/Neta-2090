#include <cstdio>

#include "core/Config.h"
#include "core/Game.h"

int main(int argc, char** argv) {
    neta::core::Config config = neta::core::Config::parse(argc, argv);
    if (config.showHelp) {
        std::puts(neta::core::Config::helpText().c_str());
        return 0;
    }
    neta::core::Game game(config);
    return game.run();
}
