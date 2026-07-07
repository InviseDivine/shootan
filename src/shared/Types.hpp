#pragma once

#include <string>
#include <cstdint>
#include <array>
#include <vector>

#define HEADER_SIZE 1
#define WORLD_SIZE 128
#define TICKS 60
#define PLAYER_SIZE 1.f

struct RVector2 {
    float x;
    float y;

    RVector2& operator=(const RVector2& other) {
        if (this != &other) {
            x = other.x;
            y = other.y;
        }
        return *this;
    }
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
    LEVEL,
    UPDATECOLLECTIBLE,
    SETSCORE,
    UPDATEANGLE,
    MESSAGE
};

struct Weapon {
    float lifeTime;
    int maxBullets;
    int damage;    
    int bulletsCount;
    float bulletSpeed;
    float reloadTime;
};

enum Block : uint8_t {
    AIR = 0,
    GRASS,
    DIRT,
    BRICK,
    LADDER,

    BLOCKS_COUNT
};

enum Weapons : uint8_t {
    GUN = 0,
    SHOTGUN,
    SNIPER_RIFLE,

    WEAPONS_COUNT
};

enum Collectibles : uint8_t {
    NONE = 0,
    SHOTGUN_COLLECT,
    SNIPER_COLLECT,
    MEDKIT,

    COLLECTIBLIES_COUNT
};

struct Message {
    std::string text;
    float lifeTime;
    unsigned char alpha;
};

struct Collectible {
    RVector2 pos;
    Collectibles type;

    float respawnTime;

    float newY;

    bool isSent;
};

struct Player {
    std::string nickname;
    
    float x, y;
    int hp;

    uint8_t currentWeapon;

    // std::array<uint8_t, WEAPONS_COUNT> inventory;
    std::vector<uint8_t> inventory;
    RVector2 speed;

    bool onGround;

    int score;  

    float reload;

    float angle;
};

struct Bullet {
    RVector2 pos;
    RVector2 velocity;

    uint32_t id;
    
    float angle;

    float lifeTime;
    
    uint32_t owner;

    uint8_t weaponId;
};