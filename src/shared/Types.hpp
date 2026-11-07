#pragma once

#include <string>
#include <cstdint>
#include <array>
#include <vector>

#define HEADER_SIZE 1
#define WORLD_SIZE 128
#define TICKS 60
#define PLAYER_SIZE 1.f

#include <fix_win32_compatibility.h>
#if defined (SHOOTAN_CLIENT)
#include <raylib.h>
#else
typedef struct Vector2 {
    float x;
    float y;
} Vector2;

typedef struct Rectangle {
    float x; 
    float y;
    float width;
    float height;
} Rectangle;
#endif

enum Header : uint8_t {
    AUTH = 0,
    ADDPLAYER,
    MOVE,
    REMOVEPLAYER,
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
    REMOVEGRENADE,
    REVIVE,
    ADDALLPLAYERS,
    SHOOT,
    ADDGRENADEINVENTORY,
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

enum GrenadeType : uint8_t {
    GRENADE_NONE = 0,
    GRENADE_COMMON,

    GRENADES_COUNT
};


struct Grenade {    
    Vector2 velocity;
    Vector2 pos;
    
    uint32_t id;
    GrenadeType grenadeId;
    uint32_t owner;
    
    float lifeTime;
};

struct GrenadeStats {
    int burstRadius;
    int damage;
    float speed;
    float lifeTime;
};

enum WeaponType : uint8_t {
    GUN = 0,
    SHOTGUN,
    SNIPER_RIFLE,

    WEAPONS_COUNT
};

enum CollectibleType : uint8_t {
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
    Vector2 pos;
};

struct Collectible {
    Vector2 pos;
    CollectibleType type;

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
    CURSOR_SPRITE,

    SPRITES_COUNT
};

enum DisconnectReason {
    WRONG_HEADER = 0,
    NICKNAME_TOO_LONG,
};

enum InputType {
    KEYBOARD = 0,
    GAMEPAD,
    TOUCH
};

struct Player {
    // TODO: Replace with Vector2
    float x, y = 0.f;
    int hp = 100;

    Hat hat = Hat::NONE_HAT;

    std::string nickname = "";

    WeaponType currentWeapon = WeaponType::GUN;

    std::array<bool, WEAPONS_COUNT> inventory = { true };

    Vector2 speed = { 0 };

    bool onGround = true;

    int score = 0;  

    std::array<float, WEAPONS_COUNT> reload = { 0 };

    float angle = 0.f;
    
    GrenadeType grenade = GrenadeType::GRENADE_NONE;

    bool onLadder = false;
    bool isDied = false;
};

struct Bullet {
    Vector2 pos;
    Vector2 velocity;

    uint32_t id;
    
    float angle;

    float lifeTime;
    
    uint32_t owner;

    uint8_t weaponId;
};

struct Packet {};

#pragma pack(push, 1)

struct LoginPacket : Packet {
    Header header = Header::AUTH;
    char username[32] = { 0 };
    Hat hat = Hat::NONE_HAT;
};

struct MovePacket : Packet {
    Header header = Header::MOVE;
    uint32_t id = 0;
    Vector2 position = { 0 };
};

struct AddBulletPacket : Packet {
    Header header = Header::ADDBULLET;

    float angle = 0.f;
    uint32_t id = 0;

    Vector2 position = { 0 };
    Vector2 velocity = { 0 };
};

struct UpdateWeaponPacket : Packet {
    Header header = Header::UPDATEWEAPON;
    uint32_t id = 0;
    WeaponType index = WeaponType::GUN;
};

struct UpdateAnglePacket : Packet {
    Header header = Header::UPDATEANGLE;
    uint32_t id = 0;
    float angle = 0.f;
};

// TODO: Collectibles
struct LevelPacket : Packet {
    Header header = Header::LEVEL;
    
    uint32_t levelSize = 0;
    uint8_t world[WORLD_SIZE * WORLD_SIZE] = { 0 };

    uint32_t backgroundSize = 0;
    uint8_t background[WORLD_SIZE * WORLD_SIZE] = { 0 };

    uint32_t collectiblesNumber = 0;
};

struct AddPlayerPacket : Packet {
    Header header = Header::ADDPLAYER;
    uint32_t peerId = 0;
    Vector2 position = { 0 };
    Hat hat = Hat::NONE_HAT;
    char nickname[32] = { 0 };
};

// TODO: Implement all players
struct AddAllPlayersPacket : Packet {
    Header header = Header::ADDALLPLAYERS;

};

// NOTE: Idk maybe we can merge packets with same vars
struct AddGrenadePacket : Packet {
    Header header = Header::ADDGRENADE;

    float angle = 0.f;
    uint32_t id = 0;

    Vector2 position = { 0 };
    Vector2 velocity = { 0 };
};

struct RevivePacket {
    Header header = Header::REVIVE;
    uint32_t id = 0;
    Vector2 position = { 0 };
};

struct SetHpPacket {
    Header header = Header::SETHP;
    uint32_t id = 0;
    int hp = 0;
};

struct RemoveBulletPacket {
    Header header = Header::REMOVEBULLET;
    uint32_t bulletId = 0;
};

struct RoundEndPacket {
    Header header = Header::ROUNDEND;
    uint32_t winner = 0;
};

struct UpdateCollectiblePacket {
    Header header = Header::UPDATECOLLECTIBLE;
    Vector2 position = { 0 };
    CollectibleType type = CollectibleType::NONE;
};

struct AddWeaponPacket {
    Header header = Header::ADDWEAPON;
    WeaponType type = WeaponType::GUN;
};

struct AddGrenadeInventoryPacket {
    Header header = Header::ADDGRENADEINVENTORY;
    GrenadeType type = GrenadeType::GRENADE_NONE;
};

struct DamagePacket {
    Header header = Header::DAMAGE;
    uint32_t damageReceiver = 0;
    int damage = 0;
};

struct ScorePacket {
    Header header = Header::SETSCORE;
    uint32_t killerId = 0;
    uint32_t deadId = 0;
    int score = 0;
};

struct RemoveGrenadePacket {
    Header header = Header::REMOVEGRENADE;
    uint32_t grenadeId = 0;
};

struct RemovePlayerPacket {
    Header header = Header::REMOVEPLAYER;
    uint32_t id = 0;
};

struct MessagePacket {
    Header header = Header::MESSAGE;
    char message[512];
};


struct ShootPacket {
    Header header = Header::SHOOT;
    float angle = 0.f;
};

struct ThrowGrenadePacket {
    Header header = Header::THROWGRENADE;
    float angle = 0.f;
};

#pragma pack(pop) 