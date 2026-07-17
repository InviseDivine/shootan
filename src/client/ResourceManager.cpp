#include <ResourceManager.hpp>

float const zoom = 0.06f;

// public
const Vector2 ResourceManager::getHatPos(Hat hat) {
    switch (hat) {
        case WIZARD_HAT: return {0, -8};
        case LUFFY_HAT: return {0, -6};
        case COOL_SUNGLASSES_HAT: return {0, 1};
        default: return {0, 0};
    }
}

const Sprite ResourceManager::getHatSprite(Hat hat) {
    switch (hat) {
        case WIZARD_HAT: return WIZARD_HAT_SPRITE;
        case LUFFY_HAT: return LUFFY_HAT_SPRITE;
        case COOL_SUNGLASSES_HAT: return COOL_SUNGLASSES_HAT_SPRITE;
        default: return SPRITES_COUNT;
    }
}

void ResourceManager::init() {
    m_spritesheet = LoadTexture("assets/spritesheet.png");
    m_blocks = LoadTexture("assets/blocks.png");
}

const Vector2 ResourceManager::getSpriteSize(Sprite index) {
    return {getSrc(index).width, getSrc(index).height};
}

void ResourceManager::drawBlock(Block block, Rectangle dest, Color color) {
    int xSrc = (block % 16) * 8.f;
    int ySrc = (block / 16) * 8.f;
    
    DrawTexturePro(m_blocks, {(float)xSrc, (float)ySrc, 8.f, 8.f}, dest, {0, 0}, 0, color);
}

void ResourceManager::drawBlockWorld(Block block, Vector2 pos, Color color) {
    drawBlock(block, {pos.x, pos.y, 1.f, 1.f}, color);
}

void ResourceManager::drawWeaponPlayer(Weapons weapon, float rotation, Vector2 pos, Color color, bool flip) {
    switch (weapon) {
        case GUN: {
            auto texFlip = flip ? -1 : 1;
            auto src = getSrc(PISTOL_SPRITE);

            Vector2 size = {src.width * zoom, src.height * zoom};
            Vector2 halfSize = { size.x / 2, size.y / 2};
            Vector2 playerCenter = {pos.x + 0.5f, pos.y + 0.5f};

            src.height *= texFlip;

            DrawTexturePro(m_spritesheet,
                src,    
                {playerCenter.x, playerCenter.y, size.x, size.y}, {0, halfSize.y}, rotation, color);
            break;
        }

        case SHOTGUN: {
            auto texFlip = flip ? -1 : 1;
            auto src = getSrc(SHOTGUN_SPRITE);

            Vector2 size = {src.width * zoom, src.height * zoom};
            Vector2 halfSize = { size.x / 2, size.y / 2};
            Vector2 playerCenter = {pos.x + 0.5f, pos.y + 0.5f};

            src.height *= texFlip;

            DrawTexturePro(m_spritesheet,
                src,
                {playerCenter.x, playerCenter.y, size.x, size.y}, {0, halfSize.y}, rotation, color);
            break;
        }

        case SNIPER_RIFLE: {
            auto texFlip = flip ? -1 : 1;
            auto src = getSrc(SNIPER_RIFLE_SPRITE);

            Vector2 size = {src.width * zoom, src.height * zoom};
            Vector2 halfSize = { size.x / 2, size.y / 2};
            Vector2 playerCenter = {pos.x + 0.5f, pos.y + 0.5f};

            src.height *= texFlip;

            DrawTexturePro(m_spritesheet,
                src,
                {playerCenter.x, playerCenter.y, size.x, size.y}, {0, halfSize.y}, rotation, color);

            break;
        }

        default: break;
    }
};

void ResourceManager::drawCollectible(Collectibles index, Vector2 pos, Color color, bool flip) {
    switch (index) {
        case SHOTGUN_COLLECT: {
            auto src = getSrc(SHOTGUN_SPRITE);
            Vector2 size = {src.width * zoom, src.height * zoom};

            drawSpriteFromSheet(SHOTGUN_SPRITE, {pos.x + size.x / 4, pos.y + size.y / 2, size.x, size.y}, {0, 0}, 0, WHITE);
            break;
        }   

        case SNIPER_COLLECT: {
            auto src = getSrc(SNIPER_RIFLE_SPRITE);
            Vector2 size = {src.width * zoom, src.height * zoom};

            drawSpriteFromSheet(SNIPER_RIFLE_SPRITE, {pos.x + size.x / 4, pos.y + size.y / 2, size.x, size.y}, {0, 0}, 0, WHITE);

            break;
        }

        case MEDKIT: {
            auto src = getSrc(MEDKIT_SPRITE);
            Vector2 size = {src.width * zoom, src.height * zoom};

            drawSpriteFromSheet(MEDKIT_SPRITE, {pos.x + size.x / 4, pos.y + size.y / 2, size.x, size.y}, {0, 0}, 0, WHITE);

            break;
        }

        case GRENADE_COLLECT :{
            auto src = getSrc(GRENADE_SPRITE);
            Vector2 size = {src.width * zoom, src.height * zoom};

            drawSpriteFromSheet(GRENADE_SPRITE, {pos.x + size.x / 4, pos.y + size.y / 2, size.x, size.y}, {0, 0}, 0, WHITE);

            break;
        }
        default: break;
    }
}

const Sprite ResourceManager::getWeaponSprite(Weapons weapon) {
    switch (weapon) {
        case GUN: return PISTOL_SPRITE;
        case SHOTGUN: return SHOTGUN_SPRITE;
        case SNIPER_RIFLE: return SNIPER_RIFLE_SPRITE;

        default: break;
    }
}

void ResourceManager::drawSpriteFromSheet(Sprite index, Rectangle dest, Vector2 origin, float rotation, Color color, bool flip) {
    DrawTexturePro(m_spritesheet, getSrc(index, flip), dest, origin, rotation, color);    
}

// private
Rectangle ResourceManager::getSrc(Sprite index, bool flip) {
    if (index < SPRITES_COUNT) {
        auto src = m_texturesSrc.at(index);

        if (flip) src.width *= -1;

        return src;
    }

    return { 0 };
}