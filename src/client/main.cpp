#include <Game.hpp>
#include <ctime>
#include <format>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

int main(int argc, char** argv) {
    std::srand(time(0));
    auto& game = Game::get();
    
    game.init(std::format("Player-{}", rand() % 1000));
    
    return 0;
}