#pragma once

#include <string>
#include <cstdint>
#include <array>

#define HEADER_SIZE 1
#define WORLD_SIZE 256
#define TICKS 60

struct RVector2 {
    float x;
    float y;
};

struct RRectangle {
    float x;
    float y;
    float width;
    float height;
};

enum Header : uint8_t {
    AUTH = 0,
    ADDPLAYER,
    MOVE,
    REMOVEPLAYER,
    LOADWORLD,
    ADDBULLET,
    REMOVEBULLET,
    SETHP,
    ADDWEAPON,
    UPDATEWEAPON,
};

struct Weapon {
    float lifeTime;
    int maxBullets;
    int damage;    
    int bulletsCount;
    float bulletSpeed;
};

enum Weapons {
    GUN = 0,
    SHOTGUN,
    SNIPER_RIFLE,

    WEAPONS_COUNT
};

enum Collectibles {
    NONE = 0,
    GUN_COLLECT,
    SHOTGUN_COLLECT,
    SNIPER_COLLECT,
    MEDKIT
};

struct Player {
    std::string nickname;
    
    float x, y;
    int hp;

    uint8_t currentWeapon;
    std::array<uint8_t, WEAPONS_COUNT> m_inventory;

    RVector2 speed;

    bool onGround;

};

struct Bullet {
    RVector2 pos;
    RVector2 velocity;

    uint32_t id;

    float lifeTime;
    uint32_t owner;

    uint8_t weaponId;
};