#pragma once

#include <Types.hpp>
#include <array>
// struct Weapon {
//     float lifeTime;
//     const int maxBullets;
//     const int damage;
//     int bulletsCount;
//     float reloadTime;
// };


inline std::array<Weapon, WEAPONS_COUNT> weapons {{
    {150.f, 20, 10, 20, 0.1f}, // GUN
    {75.f, 12, 25, 12, 0.1f},  // SHOTGUN
    {200.f, 4, 50, 4, 0.15f},   // SNIPER_RIFLE
}};