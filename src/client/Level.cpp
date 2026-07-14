#include "Types.hpp"
#include <Level.hpp>
#include <cmath>
#include <iostream>
#include <Game.hpp>
#include <Utils.hpp>
#include <fstream>
#include <ResourceManager.hpp>
#include <Weapons.hpp>
#include <Collisions.hpp>

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

    for (auto& grenade : m_grenades) {
        rm.drawSpriteFromSheet(GRENADE_SPRITE, {grenade.pos.x, grenade.pos.y, 0.5f, 0.5f});
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

    for (auto& grenade : m_grenades) {
        grenade.velocity.y += 0.02f;
        auto& stats = grenades.at(grenade.grenadeId);

        float prevX = grenade.velocity.x;
        float prevY = grenade.velocity.y;

        int blocksAroundCount = 10;
        
        std::vector<Vector2> blocksAroundArr = getBlocksAround({grenade.pos.x, grenade.pos.y}, stats.burstRadius);
        
        RRectangle grenadeBox = {grenade.pos.x, grenade.pos.y, 0.5f, 0.5f};

        float x = grenade.velocity.x;
        // Check for X collision
        for (int i = 0; i < blocksAroundArr.size(); i++) {
            x = ClipX(RRectangle{blocksAroundArr.at(i).x, blocksAroundArr.at(i).y, 1.0f, 1.0f}, grenadeBox, x);
        }
        auto tempX = grenade.pos.x + x;

        if (tempX >= 0 && tempX < WORLD_SIZE - 1) {
            grenadeBox.x = grenade.pos.x = tempX;
        }
        
        float y = grenade.velocity.y;
        // Check for Y collision
        for (int i = 0; i < blocksAroundArr.size(); i++) {
            y = ClipY(RRectangle{blocksAroundArr.at(i).x, blocksAroundArr.at(i).y, 1.0f, 1.0f}, grenadeBox, y);
        }
        grenade.pos.y += y;

        // printf("%f %f \n", x, y);

        // Stop motion on collision
        if (prevX != x) grenade.velocity.x *= -1;
        // if (prevY != y) grenade.velocity.y *= -1;

        if (grenade.velocity.x > -0.05f && grenade.velocity.x < 0.05f) grenade.velocity.x = 0;
        grenade.velocity.x *= 0.95f;
        grenade.velocity.y *= 0.99f;   
    }
}

std::vector<Vector2> Level::getBlocksAround(Vector2 pos, int radius) {
    std::vector<Vector2> blocksAround;

    int blocksAroundCount = 0;
    for (int yy = (int)pos.y - radius; yy <= (int)(pos.y + radius); yy++) {
        for (int xx = (int)pos.x - radius; xx <= (int)(pos.x + radius); xx++) {
            if (GetBlock(xx, yy)) {
                if (GetBlock(xx, yy) == LADDER) {
                    continue;
                }
                
                if (blocksAroundCount >= 15) {
                    break;
                } 

                blocksAround.push_back(Vector2{(float)xx, (float)yy});
                blocksAroundCount++;
            }
        }
    }

    return blocksAround;
}