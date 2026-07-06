#include "Types.hpp"
#include <Level.hpp>
#include <cmath>
#include <iostream>
#include <raylib.h>
#include <Game.hpp>
#include <Utils.hpp>

void Level::drawBlock(Block block, int x, int y) {
    auto& sprite = Game::get().getBlocksSprite();

    int xSrc = (block % 16) * 8.f;
    int ySrc = (block / 16) * 8.f;
    
    DrawTexturePro(sprite, {(float)xSrc, (float)ySrc, 8.f, 8.f}, {(float)x, (float)y, 1.f, 1.f}, {0, 0}, 0, WHITE);
}

void Level::render() {
    auto& game = Game::get();

    for (int y = 0; y < WORLD_SIZE; y++) {
        for (int x = 0; x < WORLD_SIZE; x++) {
            auto block = GetBlock(x, y);

            if (block) {
                drawBlock((Block)(block - 1), x, y);

                // DrawRectangle(x, y, 1, 1, YELLOW);
            }  
        }
    }

    // TODO: Fix bullet pos when shooting
    for (auto& bullet : m_bullets) {
        auto& bulletTex = game.getTexture((Collectibles) 4);

        Vector2 size = {0.3f, 0.2f};

        DrawTexturePro(bulletTex, {0, 0, (float)bulletTex.width, (float)bulletTex.height}, 
        {bullet.pos.x, bullet.pos.y, size.x, size.y}, {0, 0}, bullet.angle, WHITE);
        // DrawCircleV({}, 0.1f, BLUE);
    }
    static float timee;

    timee += GetFrameTime();

    for (auto& coll : m_collectiblies) {
        if (coll.type != 0) {       
            auto& tex = game.getTexture(coll.type);
            auto texHeight = tex.height * 0.02f;

            coll.newY = coll.newY + (float)sin(timee) * 0.004f;

            DrawTexturePro(tex, {0, 0, (float)tex.width, (float)tex.height}, 
            {coll.pos.x, coll.newY - (texHeight / 2), tex.width * 0.02f, texHeight}, {0, 0}, 0, WHITE);
        }
    }
}

void Level::update() {
    for (auto& bullet : m_bullets) {
        bullet.pos.x += bullet.velocity.x;
        bullet.pos.y += bullet.velocity.y;
    }
}