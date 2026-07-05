#include "Types.hpp"
#include <Level.hpp>
#include <cmath>
#include <iostream>
#include <raylib.h>
#include <Game.hpp>
#include <Utils.hpp>


void Level::render() {
    auto& game = Game::get();

    for (int y = 0; y < WORLD_SIZE; y++) {
        for (int x = 0; x < WORLD_SIZE; x++) {
            if (GetBlock(x, y)) {
                DrawRectangle(x, y, 1, 1, YELLOW);
            }  
        }
    }

    for (auto& bullet : m_bullets) {
        DrawCircleV({bullet.pos.x, bullet.pos.y}, 0.1f, BLUE);
    }
    static float timee;

    timee += GetFrameTime();

    for (auto& coll : m_collectiblies) {
        if (coll.type != 0) {            
            auto& tex = game.getTexture(coll.type);
            auto texHeight = tex.height * 0.02f;

            
            coll.pos.y = coll.pos.y + (float)sin(timee) * 0.004f;

            DrawTexturePro(tex, {0, 0, (float)tex.width, (float)tex.height}, 
            {coll.pos.x, coll.pos.y - (texHeight / 2), tex.width * 0.02f, texHeight}, {0, 0}, 0, WHITE);
        }
    }
}

void Level::update() {
    for (auto& bullet : m_bullets) {
        bullet.pos.x += bullet.velocity.x;
        bullet.pos.y += bullet.velocity.y;
    }
}