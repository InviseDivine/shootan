#pragma once

#include <Types.hpp>
#include <array>

// struct GrenadeStats {
//     int burstRadius;
//     int damage;
//     float speed;
//     float lifeTime;
// };

// struct Weapon {
//     float lifeTime;
//     int maxBullets;
//     int damage;    
//     int bulletsCount;
//     float bulletSpeed;
//     float reloadTime;
// };

inline std::array<Weapon, WEAPONS_COUNT> weapons {{
    {175.f, 20, 10, 20, 0.5f, 12.f}, // GUN
    {75.f, 12, 15, 12, 0.6f, 125.f},  // SHOTGUN
    {225.f, 4, 50, 4, 0.7f, 200.f},   // SNIPER_RIFLE
}};

inline std::array<GrenadeStats, GRENADES_COUNT - 1> grenades {{
    {3, 50, 0.7f, 180.f}, // COMMON
}};