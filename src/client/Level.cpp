#include "Types.hpp"
#include <Level.hpp>
#include <cmath>
#include <iostream>
#include <Game.hpp>
#include <Utils.hpp>
#include <fstream>
#include <ResourceManager.hpp>

#define ONSCREEN(maxX, maxY, minX, minY, x, y) y >= minY && y <= maxY && x >= minX && x <= maxX


void Level::write() {
    std::ofstream world("world.dat", std::ios::binary);

    if (world) {
        uint32_t collSize = (uint32_t)m_collectiblies.size();
        uint32_t spawnSize = (uint32_t)m_respawnPoints.size();

        world.write(reinterpret_cast<const char*>(m_world.data()), WORLD_SIZE * WORLD_SIZE);
        world.write(reinterpret_cast<const char*>(m_background.data()), WORLD_SIZE * WORLD_SIZE);
        world.write(reinterpret_cast<const char*>(&collSize), sizeof(collSize));
        world.write(reinterpret_cast<const char*>(m_collectiblies.data()), m_collectiblies.size() * sizeof(Collectible));
        world.write(reinterpret_cast<const char*>(&spawnSize), sizeof(spawnSize));
        world.write(reinterpret_cast<const char*>(m_respawnPoints.data()), m_respawnPoints.size() * sizeof(RVector2));
        
        world.close();
    }
}

void Level::read() {
    std::ifstream world("world.dat", std::ios::binary);
    std::cout << "reading" << std::endl;

    if (world) {
        world.read(reinterpret_cast<char*>(m_world.data()),
            WORLD_SIZE * WORLD_SIZE);

        world.read(reinterpret_cast<char*>(m_background.data()),
            WORLD_SIZE * WORLD_SIZE); 

        uint32_t collSize = 0;

        world.read(reinterpret_cast<char*>(&collSize), sizeof(collSize));

        m_collectiblies.resize(collSize);

        world.read(reinterpret_cast<char*>(m_collectiblies.data()), collSize * sizeof(Collectible));

        uint32_t spawnpointSize = 0;

        world.read(reinterpret_cast<char*>(&spawnpointSize), sizeof(spawnpointSize));

        m_respawnPoints.resize(spawnpointSize);

        world.read(reinterpret_cast<char*>(m_respawnPoints.data()), spawnpointSize * sizeof(RVector2));

        world.close();

        for (auto& coll : m_collectiblies) {
            coll.newY = coll.pos.y;
        }
    }   
}
void Level::drawBlock(Block block, int x, int y, Color color) {
    auto& rm = ResourceManager::get();

    rm.drawBlockWorld(block, {(float)x, (float)y}, color);
}

void Level::render() {
    auto& game = Game::get();

    Vector2 min = GetScreenToWorld2D({0.f, 0.f}, game.getCamera());
    Vector2 max = GetScreenToWorld2D({(float)GetScreenWidth(), (float)GetScreenHeight()}, game.getCamera());
    auto& rm = ResourceManager::get();

    for (int y = min.y; y < max.y; y++) {
        for (int x = min.x; x < max.x; x++) {
            auto block = GetBackgroundBlock(x, y);

            if (block) {
                drawBlock((Block)(block - 1), x, y, GRAY);
            }  
        }
    }

    for (int y = min.y; y < max.y; y++) {
        for (int x = min.x; x < max.x; x++) {
            auto block = GetBlock(x, y);

            if (block) {
                drawBlock((Block)(block - 1), x, y);
            }  
        }
    }

    for (auto& bullet : m_bullets) {
        if (ONSCREEN(max.x, max.y, min.x, min.y, bullet.pos.x, bullet.pos.y)) {
            rm.drawSpriteFromSheet(BULLET_SPRITE, {bullet.pos.x, bullet.pos.y, 0.3f, 0.2f}, {0, 0}, bullet.angle, WHITE);
        }
    }
    
    static float timee;

    timee += GetFrameTime();

    for (auto& coll : m_collectiblies) {
        // auto& tex = game.getTexture(coll.type);
        // auto texHeight = tex.height * 0.02f;

        // if (
        //     (ONSCREEN(max.x, max.y, min.x, min.y, coll.pos.x + tex.width * 0.02f, coll.pos.y + texHeight) ||
        //     ONSCREEN(max.x, max.y, min.x, min.y, coll.pos.x - tex.width * 0.02f, coll.pos.y - texHeight))

        // && coll.type != 0) {       

        coll.newY = coll.newY + (float)sin(timee) * 0.002f;

            // DrawTexturePro(tex, {0, 0, (float)tex.width, (float)tex.height}, 
            // {coll.pos.x, coll.newY - (texHeight / 2), tex.width * 0.02f, texHeight}, {0, 0}, 0, WHITE);
        // }
        
        rm.drawCollectible(coll.type, {coll.pos.x, coll.newY}, WHITE);  
    }

    if (game.getEditor()) {
        for (auto& [x, y] : m_respawnPoints) {
            if (ONSCREEN(max.x, max.y, min.x, min.y, x, y)) {
                DrawRectangleLinesEx({x, y, 1.f, 1.f}, 0.2f, MAROON);
            }
        }
    }
}

void Level::update() {
    for (auto& bullet : m_bullets) {
        bullet.pos.x += bullet.velocity.x;
        bullet.pos.y += bullet.velocity.y;
    }
}