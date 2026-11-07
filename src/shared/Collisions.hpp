#pragma once

#include <Types.hpp>
#include <algorithm>

inline float ClipX(Rectangle rec1, Rectangle rec2, float xa) {
    if (rec2.y + rec2.height <= rec1.y || rec2.y >= rec1.y + rec1.height) {
        return xa;
    }

    if (xa > 0.0f && rec2.x + rec2.width <= rec1.x)
        xa = std::min(rec1.x - (rec2.x + rec2.width), xa);

    if (xa < 0.0f && rec2.x >= rec1.x + rec1.width)
        xa = std::max(xa, (rec1.x + rec1.width) - rec2.x);

    return xa;
}

inline float ClipY(Rectangle rec1, Rectangle rec2, float ya) {
    if (rec2.x + rec2.width <= rec1.x || rec2.x >= rec1.x + rec1.width) {
        return ya;
    }

    if (ya > 0.0f && rec2.y + rec2.height <= rec1.y)
        ya = std::min(ya, rec1.y - (rec2.y + rec2.height));

    if (ya < 0.0f && rec2.y >= rec1.y + rec1.height)
        ya = std::max(ya, (rec1.y + rec1.height) - rec2.y);

    return ya;
}

// inline Vector2 MoveInWorld(float speedX, float speedX, Level& level, std::vector<RRectangle> hitboxes) {
//     auto prevX = speedX;
//     auto prevY = speedY;
//     auto hitboxes = level.get(Expand(x, y));

//     // Check for X collision
//     for (auto& hitbox : hitboxes) {
//         x = hitbox.ClipCollideX(*this, x);
//     }
//     this->x += x;

//     // Check for Y collision
//     for (auto& hitbox : hitboxes) {
//         y = hitbox.ClipCollideY(*this, y);
//     }
//     this->y += y;

//     // Update on ground state
//     m_onGround = prevY != y && prevY > 0.f;

//     // Stop motion on collision
//     if (prevX != x) m_speed.x = 0.f;
//     if (prevY != y) m_speed.y = 0.f;
// }