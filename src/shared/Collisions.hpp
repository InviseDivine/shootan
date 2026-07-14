#pragma once

#include <Types.hpp>
#include <Utils.hpp>

inline float ClipX(RRectangle rec1, RRectangle rec2, float xa) {
    if (rec2.y + rec2.height <= rec1.y || rec2.y >= rec1.y + rec1.height) {
        return xa;
    }

    if (xa > 0.0f && rec2.x + rec2.width <= rec1.x)
        xa = MIN(rec1.x - (rec2.x + rec2.width), xa);

    if (xa < 0.0f && rec2.x >= rec1.x + rec1.width)
        xa = MAX(xa, (rec1.x + rec1.width) - rec2.x);

    return xa;
}

inline float ClipY(RRectangle rec1, RRectangle rec2, float ya) {
    if (rec2.x + rec2.width <= rec1.x || rec2.x >= rec1.x + rec1.width) {
        return ya;
    }

    if (ya > 0.0f && rec2.y + rec2.height <= rec1.y)
        ya = MIN(ya, rec1.y - (rec2.y + rec2.height));

    if (ya < 0.0f && rec2.y >= rec1.y + rec1.height)
        ya = MAX(ya, (rec1.y + rec1.height) - rec2.y);

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