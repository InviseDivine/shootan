#pragma once

#include <Utils.hpp>
#include <Types.hpp>
#include <random>
#include <vector>
#include <array>

class Level {
public:
    Level();
    
    void update();

    inline uint8_t GetBlock(int x, int y) {
        int convX = MAX(0, x);
        int convY = MAX(0, y);

        if ((convX < WORLD_SIZE && convY < WORLD_SIZE)) {
            return m_world.at(PACK_INDEX(convX, convY, WORLD_SIZE));
        } else {
            return 0;
        }
    }

    std::array<uint8_t, WORLD_SIZE * WORLD_SIZE>& getWorld() { return m_world; }
    std::vector<Collectible>& getCollectibles() { return m_collectibles; }
    void addBullet(Bullet bullet) { m_bullets.push_back(bullet); }
    
    void addPoint(RVector2 pos) { m_respawnPoints.push_back(pos); }

    std::vector<RVector2>& getSpawnPoins() { return m_respawnPoints; }

    RVector2& getRandomSpawn() { 
        std::uniform_int_distribution<int> distrib(0, m_respawnPoints.size() - 1);

        return m_respawnPoints.at(distrib(m_gen)); 
    }
    int bulletSize() { return m_bullets.size(); }

    void read(const std::string& filepath);
private:
    std::array<uint8_t, WORLD_SIZE * WORLD_SIZE> m_world;
    std::array<uint8_t, WORLD_SIZE * WORLD_SIZE> m_background;

    std::mt19937 m_gen;

    uint32_t m_scoreLimit;
    
    std::vector<Bullet> m_bullets;
    std::vector<Collectible> m_collectibles;
    std::vector<RVector2> m_respawnPoints;
};