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
    MESSAGE, 
    DAMAGE,
    ROUNDEND,
    THROWGRENADE,
    ADDGRENADE,
    REMOVEGRENADE
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
    STONE_BRICK,
    GLASS,
    LEAVES,
    WOOD,
    BOX,

    BLOCKS_COUNT
};

enum Grenades : uint8_t {
    GRENADE_NONE = 0,
    COMMON,

    GRENADES_COUNT
};


struct Grenade {    
    RVector2 velocity;
    RVector2 pos;
    
    Grenades grenadeId;
    uint32_t id;
    uint32_t owner;
    
    float lifeTime;
};

struct GrenadeStats {
    int burstRadius;
    int damage;
    float speed;
    float lifeTime;
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
    GRENADE_COLLECT,

    COLLECTIBLIES_COUNT
};

struct Message {
    std::string text;
    float lifeTime;
    unsigned char alpha;
    RVector2 pos;
};

struct Collectible {
    RVector2 pos;
    Collectibles type;

    float respawnTime;

    float newY;

    bool isSent;
};

enum Hat : uint8_t {
    NONE_HAT = 0,
    WIZARD_HAT,
    LUFFY_HAT,
    COOL_SUNGLASSES_HAT,

    HATS_COUNT
};

enum Sprite {
    PISTOL_SPRITE = 0,
    SHOTGUN_SPRITE,
    BULLET_SPRITE,
    SNIPER_RIFLE_SPRITE,
    PLAYER_SPRITE,
    WIZARD_HAT_SPRITE,
    MEDKIT_SPRITE,
    LUFFY_HAT_SPRITE,
    COOL_SUNGLASSES_HAT_SPRITE,
    CROWN_SPRITE,
    GRENADE_SPRITE,

    SPRITES_COUNT
};

enum DisconnectReason {
    WRONG_HEADER = 0,
    NICKNAME_TOO_LONG,
};

struct Player {
    std::string nickname;
    
    float x, y;
    int hp;

    Weapons currentWeapon;

    std::array<bool, WEAPONS_COUNT> inventory;

    RVector2 speed;

    bool onGround;

    int score;  

    float reload;

    float angle;
    
    Hat hat;

    Grenades grenade;
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