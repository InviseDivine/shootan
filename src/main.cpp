#include <Game.hpp>
#include <ctime>
#include <format>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

int main(int argc, char** argv) {
    std::srand(time(0));

    std::string nickname = argc > 1 ? argv[1] : std::format("Player-{}", rand() % 1000);
    
    auto& game = Game::get();
    
    game.init(nickname);
    
    return 0;
}