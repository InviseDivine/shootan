#pragma once

#include <Utils.hpp>
#include <Types.hpp>
#include <vector>
#include <array>

class Level {
public:
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
    int bulletSize() { return m_bullets.size(); }
private:
    std::array<uint8_t, WORLD_SIZE * WORLD_SIZE> m_world;
    
    uint32_t m_scoreLimit;
    
    std::vector<Bullet> m_bullets;
    std::vector<Collectible> m_collectibles;
};