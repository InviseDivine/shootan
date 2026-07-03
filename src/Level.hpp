#pragma once
#include <array>
#include <cstdint>
#include <Types.hpp>
#include <Utils.hpp>
#include <vector>

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
            return 0;
        }
    }

    std::array<uint8_t, WORLD_SIZE * WORLD_SIZE>& getWorld() { return m_world; }

    void addBullet(Bullet bullet) { m_bullets.push_back(bullet); }
    void removeBullet(uint32_t index) {
        std::erase_if(m_bullets, [&index](Bullet blt) { 
            return blt.id == index;
        });
    }
private:
    std::array<uint8_t, WORLD_SIZE * WORLD_SIZE> m_world;
    std::vector<Bullet> m_bullets;
};