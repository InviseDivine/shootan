#pragma once
#include <array>
#include <cstdint>
#include <Types.hpp>
#include <Utils.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <raylib.h>

class Level {
public:
    void update();
    void render();

    inline uint8_t GetBlock(int x, int y) {
        int convX = MAX(0, x);
        int convY = MAX(0, y);

        if ((convX < WORLD_SIZE && convY < WORLD_SIZE)) {
            return m_world.at(PACK_INDEX(convX, convY, WORLD_SIZE));
        } else {
            return AIR;
        }
    }

    inline uint8_t GetBackground(int x, int y) {
        int convX = MAX(0, x);
        int convY = MAX(0, y);

        if ((convX < WORLD_SIZE && convY < WORLD_SIZE)) {
            return m_background.at(PACK_INDEX(convX, convY, WORLD_SIZE));
        } else {
            return AIR;
        }
    }

    inline uint8_t GetBackgroundBlock(int x, int y) {
        int convX = MAX(0, x);
        int convY = MAX(0, y);

        if ((convX < WORLD_SIZE && convY < WORLD_SIZE)) {
            return m_background.at(PACK_INDEX(convX, convY, WORLD_SIZE));
        } else {
            return AIR;
        }
    }

    inline void setBackgroundBlock(Block block, int x, int y) {
        if (x >= 0 && x < WORLD_SIZE && y >= 0 && y < WORLD_SIZE) {
            m_background.at(PACK_INDEX(x, y, WORLD_SIZE)) = block; 
        } 
    }

    inline void setBlock(Block block, int x, int y) { 
        if (x >= 0 && x < WORLD_SIZE && y >= 0 && y < WORLD_SIZE) {
            m_world.at(PACK_INDEX(x, y, WORLD_SIZE)) = block; 
        } 
    } 

    void addCollectible(Collectible coll) { coll.newY = coll.pos.y; m_collectiblies.push_back(coll); }
    void editCollectible(RVector2 pos, Collectibles type) {        
        for (int i = 0; i < m_collectiblies.size(); i++) {
            auto& coll = m_collectiblies.at(i);
            auto& pss = coll.pos;      

            if (pss.x == pos.x && pss.y == pos.y) {
                coll.type = type;
                break;
            }
        }
    }
    void removeCollectible(RVector2 pos) {   
        std::erase_if(m_collectiblies, [&pos](Collectible coll) { 
            return coll.pos.x == pos.x && coll.pos.y == pos.y;
        });     
    }

    bool containsCollectible(RVector2 pos) {
        for (int i = 0; i < m_collectiblies.size(); i++) {
            if (m_collectiblies.at(i).pos.x == pos.x && m_collectiblies.at(i).pos.y == pos.y) {
                return true;
            }
        }

        return false;
    }

    std::array<uint8_t, WORLD_SIZE * WORLD_SIZE>& getWorld() { return m_world; }
    void setWorld(std::vector<uint8_t> data) { std::copy(data.begin(), data.end(), m_world.begin()); }
    void setBackground(std::vector<uint8_t> data) { std::copy(data.begin(), data.end(), m_background.begin()); }

    void addBullet(Bullet bullet) { m_bullets.push_back(bullet); }
    void removeBullet(uint32_t index) {
        std::erase_if(m_bullets, [&index](Bullet blt) { 
            return blt.id == index;
        });
    }

    bool containsSpawnpoint(RVector2 pos) {   
        for (int i = 0; i < m_respawnPoints.size(); i++) {
            if (m_respawnPoints.at(i).x == pos.x && m_respawnPoints.at(i).y == pos.y) {
                return true;
            }
        }

        return false;
    }

    void setSpawnpoint(RVector2 pos) {
        m_respawnPoints.push_back(pos);
    }

    void removeSpawnpoint(RVector2 pos) {
        std::erase_if(m_respawnPoints, [&pos](RVector2 spawn) { 
            return spawn.x == pos.x && spawn.y == pos.y;
        });
    }

    std::vector<RVector2>& getSpawnpoints() { return m_respawnPoints; }
    void drawBlock(Block block, int x, int y, Color color = WHITE);
    
    void read();
    void write();

    void addGrenade(Grenade grenade) { m_grenades.push_back(grenade); }
    void removeGrenade(uint32_t id) { 
        std::erase_if(m_grenades, [&id](Grenade grenade) { 
            return grenade.id == id;
        });
    }
    std::vector<Vector2> getBlocksAround(Vector2 pos, int radius);
private:
    std::array<uint8_t, WORLD_SIZE * WORLD_SIZE> m_world;
    std::array<uint8_t, WORLD_SIZE * WORLD_SIZE> m_background;

    std::vector<RVector2> m_respawnPoints;
    std::vector<Bullet> m_bullets;
    std::vector<Collectible> m_collectiblies;
    std::vector<Grenade> m_grenades;
};