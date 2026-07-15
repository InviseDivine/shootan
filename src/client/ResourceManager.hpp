#pragma once
#include <array>
#include "Types.hpp"
#include <raylib.h>

// PISTOL_SPRITE = 0,
// SHOTGUN_SPRITE,
// BULLET_SPRITE,
// SNIPER_RIFLE_SPRITE,
// PLAYER_SPRITE,
// WIZARD_HAT_SPRITE,

class ResourceManager {
public:
    static inline ResourceManager& get() {
        static ResourceManager inst;
        return inst;
    }
    
    void init();
    
    void drawSpriteFromSheet(Sprite index, Rectangle dest, Vector2 origin = {0, 0}, float rotation = 0, Color color = WHITE, bool flip = false);

    void drawWeaponPlayer(Weapons weapon, float rotation, Vector2 pos, Color color = WHITE, bool flip = false);
    void drawCollectible(Collectibles index, Vector2 pos, Color color = WHITE, bool flip = false);

    void drawBlockWorld(Block block, Vector2 pos, Color color = WHITE);
    void drawBlock(Block block, Rectangle dest, Color color = WHITE);

    bool isSpriteOnScreen(Collectible coll); 

    const Vector2 getSpriteSize(Sprite index);

    const Sprite getWeaponSprite(Weapons weapon);

    const Sprite getHatSprite(Hat hat);

    const Vector2 getHatPos(Hat hat);
private:
    Texture2D m_spritesheet;
    Texture2D m_blocks;
    Rectangle getSrc(Sprite index, bool flip = false);

    std::array<Rectangle, SPRITES_COUNT> m_texturesSrc {{
        {0, 0, 20, 12},  // PISTOL
        {20, 0, 24, 8},  // SHOTGUN_SPRITE
        {0, 20, 8, 6},   // BULLET_SPRITE
        {0, 12, 32, 8},  // SNIPER_RIFLE_SPRITE
        {32, 8, 8, 8},   // PLAYER_SPRITE
        {40, 8, 8, 8},   // WIZARD_HAT_SPRITE
        {0, 27, 11, 10}, // MEDKIT_SPRITE
        {44, 0, 8, 6},   // LUFFY_HAT_SPRITE
        {52, 0, 7, 6},   // COOL_SUNGLASSES_HAT_SPRITE
        {8, 20, 7, 7},   // CROWN_SPRITE
        {11, 32, 5, 5},  // GRENADE_SPRITE
    }};
};