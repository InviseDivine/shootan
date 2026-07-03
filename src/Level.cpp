#include <Level.hpp>
#include <raylib.h>

void Level::render() {
    for (int y = 0; y < WORLD_SIZE; y++) {
        for (int x = 0; x < WORLD_SIZE; x++) {
            if (GetBlock(x, y)) {
                DrawRectangle(x, y, 1, 1, YELLOW);
            }  
        }
    }

    for (auto& bullet : m_bullets) {
        DrawCircleV({bullet.pos.x, bullet.pos.y}, 5.f, BLUE);
    }
}

void Level::update() {
    for (auto& bullet : m_bullets) {
        bullet.pos.x += bullet.velocity.x;
        bullet.pos.y += bullet.velocity.y;
    }
}