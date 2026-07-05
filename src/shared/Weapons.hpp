#pragma once

#include <Types.hpp>
#include <array>

// struct Weapon {
//     float lifeTime;
//     int maxBullets;
//     int damage;    
//     int bulletsCount;
//     float bulletSpeed;
//     float reloadTime;
// };

inline std::array<Weapon, WEAPONS_COUNT> weapons {{
    {175.f, 20, 10, 20, 0.25f, 12.f}, // GUN
    {75.f, 12, 25, 12, 0.3f, 25.f},  // SHOTGUN
    {225.f, 4, 50, 4, 0.5f, 75.f},   // SNIPER_RIFLE
}};