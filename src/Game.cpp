#include "Types.hpp"
#include "Multiplayer.hpp"
#include <Game.hpp>
#include <cmath>
#include <Utils.hpp>
#include <Collisions.hpp>
#include <raylib.h>
#include <raymath.h>
#include <thread>
#include <Weapons.hpp>
#include <raygui.h>

void Game::sendMovePacket() {
    auto moveSize = HEADER_SIZE + sizeof(float) * 2;
    auto movePacket = new char[moveSize];
    auto& mp = Multiplayer::get();
    
    movePacket[0] = MOVE;
    *(float*)(movePacket + 1) = m_player.x;
    *(float*)(movePacket + 5) = m_player.y;

    mp.sendPacket(movePacket, moveSize, true);

    delete [] movePacket;
}
void Game::init(std::string nickname) {
    InitWindow(1280, 720, "Shootan YOOO");

    SetTargetFPS(60);

    m_editor = 1;

    m_player = {nickname, 1, 61, 100, 0, {GUN}, {0, 0}, true};

    // TODO: ResourceManager
    m_textures.at(GUN) = LoadTexture("assets/pistol.png");
    m_textures.at(SHOTGUN) = LoadTexture("assets/shotgun.png");
    m_textures.at(SNIPER_RIFLE) = LoadTexture("assets/sniper.png");
    m_textures.at(SNIPER_RIFLE + 1) = LoadTexture("assets/player.png");
    m_textures.at(SNIPER_RIFLE + 2) = LoadTexture("assets/bullet.png");
    m_blocks = LoadTexture("assets/blocks.png");
    
    SetTextureFilter(m_textures.at(GUN), TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(m_textures.at(SHOTGUN), TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(m_textures.at(SNIPER_RIFLE), TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(m_blocks, TEXTURE_FILTER_POINT);
    Image icon = LoadImage("assets/ico.png");    
    SetWindowIcon(icon);
    UnloadImage(icon);    

    if (!m_editor) {
        auto& mp = Multiplayer::get();
        std::thread(&Multiplayer::init, &mp, m_player.nickname, "localhost", 6890).detach();
    } else {
        m_level.read();
    }

    m_camera = { 0 };

    m_camera.offset = Vector2{ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    m_camera.rotation = 0.0f;
    m_camera.zoom = 50.0f;
    m_cameraPos = {64, 120};

    while (!WindowShouldClose()) {
        if (m_editor) {
            updateEditor();
        } else {
            update();
        }

        BeginDrawing();
            if (m_editor) {
                renderEditor();
            } else {
                render();
            }
        EndDrawing();
    }
}
void Game::update() {
    m_timer.advanceTime();

    auto width = GetScreenWidth();
    auto height = GetScreenHeight();

    m_camera.offset = Vector2 { width / 2.0f, height / 2.0f };
    m_camera.target = Vector2 {m_player.x, m_player.y};
    
    Vector2 max = GetWorldToScreen2D(Vector2 { WORLD_SIZE, WORLD_SIZE }, m_camera);
    Vector2 min = GetWorldToScreen2D(Vector2 { 0, 0 }, m_camera);

    if (max.x < width) m_camera.offset.x = width - (max.x - (float)width/2);
    if (max.y < height) m_camera.offset.y = height - (max.y - (float)height/2);
    if (min.x > 0) m_camera.offset.x = (float)width/2 - min.x;
    if (min.y > 0) m_camera.offset.y = (float)height/2 - min.y;

    for (uint32_t i = 0; i < m_timer.getTicks(); i++) {
        m_level.update();

        if (m_player.reload > 0) {
            m_player.reload -= 1.f;
        }
    }
        
    for (uint8_t i = 0; i < m_player.inventory.size(); i++) {
        if (IsKeyPressed(KEY_ONE + i)) {
            auto updateWeapon = new char[HEADER_SIZE + sizeof(uint8_t)];
            updateWeapon[0] = UPDATEWEAPON;
            updateWeapon[1] = i;
            
            auto& mp = Multiplayer::get();
            
            mp.sendPacket(updateWeapon, 2, true);
            
            delete [] updateWeapon;
        }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {        
        Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), m_camera);

        Vector2 gunPos = {m_player.x + 0.5f, m_player.y + 0.5f};

        Vector2 direction = Vector2Subtract(mousePos, gunPos);

        float angle = atan2f(direction.y, direction.x);

        auto& mp = Multiplayer::get();

        auto addBulletPacket = new char[HEADER_SIZE + 4];
        addBulletPacket[0] = Header::ADDBULLET;

        *(float*)(addBulletPacket + HEADER_SIZE) = angle;

        mp.sendPacket(addBulletPacket, HEADER_SIZE + 4, true);

        m_player.reload = weapons.at(m_player.inventory.at(m_player.currentWeapon)).reloadTime;

        delete [] addBulletPacket;
    }
    
    auto onLadder = m_level.GetBlock(std::ceil(m_player.x), std::ceil(m_player.y)) == LADDER ||
        m_level.GetBlock(std::floor(m_player.x), std::floor(m_player.y)) == LADDER;

    if ((IsKeyDown(KEY_W) || IsKeyDown(KEY_SPACE) ||  IsKeyDown(KEY_UP))) {
        if (onLadder) {
            // TODO: Rewrite
            int playerX = m_level.GetBlock(std::ceil(m_player.x), std::ceil(m_player.y)) == LADDER ? std::ceil(m_player.x) :
            m_level.GetBlock(std::floor(m_player.x), std::floor(m_player.y)) == LADDER ? std::floor(m_player.x) : 0;

            m_player.speed.y = -0.2f;
            m_player.x = playerX;
        } else if (m_player.onGround) {
            m_player.speed.y = -0.3f;
        }
    }

    if (onLadder && (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))) {
        int playerX = m_level.GetBlock(std::ceil(m_player.x), std::ceil(m_player.y)) == LADDER ? std::ceil(m_player.x) :
        m_level.GetBlock(std::floor(m_player.x), std::floor(m_player.y)) == LADDER ? std::floor(m_player.x) : 0;

        m_player.speed.y = 0.2f;
        m_player.x = playerX;
    }
    if (IsKeyDown(KEY_A)) {
        m_player.speed.x = -0.175f;
    }

    if (IsKeyDown(KEY_D)) {
        m_player.speed.x = 0.175f;
    }
    
    // std::cout << m_player.x << std::endl;
    // std::cout << m_player.y << std::endl;

    if (m_loaded) {
        if (m_level.GetBlock(std::ceil(m_player.x), std::ceil(m_player.y)) != LADDER ||
            m_level.GetBlock(std::floor(m_player.x), std::floor(m_player.y)) != LADDER
        ) {
            m_player.speed.y += 0.02f;
        }

        float prevX = m_player.speed.x;
        float prevY = m_player.speed.y;

        Vector2 blocksAroundArr[10] = { 0 };
        
        int blocksAroundCount = 0;
        for (int yy = (int)m_player.y - 1; yy <= (int)(m_player.y + 5.f); yy++) {
            for (int xx = (int)m_player.x - 1; xx <= (int)(m_player.x + 5.f); xx++) {
                if (m_level.GetBlock(xx, yy)) {
                    if (m_level.GetBlock(xx, yy) == LADDER) {
                        continue;
                    }
                    
                    if (blocksAroundCount >= 10) {
                        break;
                    } 

                    blocksAroundArr[blocksAroundCount] = Vector2{(float)xx, (float)yy};
                    blocksAroundCount++;
                }

                if (blocksAroundCount >= 20) break;
            }
        }
        
        RRectangle playerBox = {m_player.x, m_player.y, 1.f, 1.f};

        float x = m_player.speed.x;
        // Check for X collision
        for (int i = 0; i < blocksAroundCount; i++) {
            x = ClipX(RRectangle{blocksAroundArr[i].x, blocksAroundArr[i].y, 1.0f, 1.0f}, playerBox, x);
        }
        auto tempX = m_player.x + x;

        if (tempX >= 0 && tempX < WORLD_SIZE - 1) {
            playerBox.x = m_player.x = tempX;
        }

        // printf("%f \n", x);
        float y = m_player.speed.y;
        // Check for Y collision
        for (int i = 0; i < blocksAroundCount; i++) {
            y = ClipY(RRectangle{blocksAroundArr[i].x, blocksAroundArr[i].y, 1.0f, 1.0f}, playerBox, y);
        }
        m_player.y += y;
        // printf("%f \n", y);

        m_player.onGround = prevY != y && prevY > 0.f;
        // Stop motion on collision
        if (prevX != x) m_player.speed.x = 0.f;
        if (prevY != y) m_player.speed.y = 0.f;

        if (m_player.speed.x != 0 || m_player.speed.y != 0) sendMovePacket();

        if (m_player.speed.x < 0.001f) m_player.speed.x = 0;
        m_player.speed.x *= 0.91f;
        m_player.speed.y *= 0.98f;
    }
}
void Game::render() {
    ClearBackground({4, 4, 50, 255});
    
    Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), m_camera);

    Vector2 origin = GetScreenToWorld2D({45.f, 45.f}, m_camera);

    Vector2 direction = Vector2Subtract(mousePos, {m_player.x + 0.5f, m_player.y + 0.5f});

    float anglePrev = 0;
    float angle = atan2f(direction.y, direction.x) * RAD2DEG;
    
    if (anglePrev != angle) {
        anglePrev = angle;

        auto angleSize = HEADER_SIZE + sizeof(angle);
        auto anglePacket = new char[angleSize];
        auto& mp = Multiplayer::get();
        
        anglePacket[0] = UPDATEANGLE;
        *(float*)(anglePacket + HEADER_SIZE) = angle;

        mp.sendPacket(anglePacket, angleSize, true);

        delete [] anglePacket;
    }

    int flip = !(angle >= -90 && angle < 90) ? -1 : 1;
    
    BeginMode2D(m_camera);
        m_level.render();
        auto& plrTex = m_textures.at(SNIPER_RIFLE + 1);

        for (auto& [_, client] : m_players) {  
            auto& tex = m_textures.at(client.currentWeapon);

            auto fontSize = 0.5f;
            auto text = TextFormat("%s %d", client.nickname.c_str(), client.hp);
            float spacing = 0.05f;

            int flipClient = client.angle >= 90 && client.angle < 270 ? -1 : 1;

            DrawTexturePro(plrTex, {0, 0, (float)plrTex.width * flipClient, (float)plrTex.height}, 
            {client.x, client.y, 1.f, 1.f}, {0, 0}, 0, WHITE);
            // DrawRectangleRec({client.x, client.y, 1.f, 1.f}, MAROON);   
            DrawTexturePro(tex, {0, 0, static_cast<float>(tex.width), static_cast<float>(tex.height * flipClient)},
                {client.x + 0.5f, client.y + 0.5f, 1.5f, 1.5f}, {0.75f, 0.75f}, client.angle, WHITE);
            DrawTextPro(GetFontDefault(), text, {client.x, client.y - 0.55f}, {0, 0}, 0, fontSize, spacing, WHITE);
        }
        
        
        DrawTexturePro(plrTex, {0, 0, (float)plrTex.width * flip, (float)plrTex.height}, {m_player.x, m_player.y, 1.f, 1.f}, {0, 0}, 0, WHITE);
        // DrawRectangleRec({m_player.x, m_player.y, 1.f, 1.f}, MAROON);   
        
        auto& tex = m_textures.at(m_player.inventory.at(m_player.currentWeapon));
        DrawTexturePro(tex, {0, 0, static_cast<float>(tex.width), static_cast<float>(tex.height * flip)},
            {m_player.x + 0.5f, m_player.y + 0.5f, 1.5f, 1.5f}, {0.75f, 0.75f}, angle, WHITE);
    EndMode2D();
    
    // Debug info
    DrawFPS(0, 0);
    DrawText(TextFormat("%f\n%f", m_player.x, m_player.y), 0, 20, 20, WHITE);

    if (m_player.reload > 0) {
        DrawText("Reloading...", 0, 80, 40, WHITE);
    }

    auto hpText = TextFormat("HP: %d", m_player.hp);

    DrawText(hpText, GetScreenWidth() - MeasureText(hpText, 20), 0, 20, WHITE);

    if (IsKeyDown(KEY_TAB)) {
        float x = 75;
        float y = 75;

        float width = GetScreenWidth() - x * 2;
        float height = GetScreenHeight() - y * 2;
        
        Rectangle bg = {x, y, width, height};

        DrawRectangleRec(bg, {0, 0, 0, 180});
        auto count = 0;

        DrawText("Score", width - x - 10, y + 10, 40, WHITE);
        
        DrawText(TextFormat("%s - %d", m_player.nickname.c_str(), m_player.score), x + 40, y + 40, 20, WHITE);

        for (auto& [_, client] : m_players) {
            DrawText(TextFormat("%s - %d", client.nickname.c_str(), client.score), x + 40, y + 20 * count + 60, 20, WHITE);
            count++;
        }
    }
}

void Game::updateEditor() {
    auto width = GetScreenWidth();
    auto height = GetScreenHeight();

    m_camera.offset = Vector2 { width / 2.0f, height / 2.0f };
    m_camera.target = Vector2 {m_cameraPos.x, m_cameraPos.y};
    
    Vector2 max = GetWorldToScreen2D(Vector2 { WORLD_SIZE, WORLD_SIZE }, m_camera);
    Vector2 min = GetWorldToScreen2D(Vector2 { 0, 0 }, m_camera);

    Vector2 mousePos = GetMousePosition();
    Vector2 worldMouse = GetScreenToWorld2D(mousePos, m_camera);
    Vector2 worldMousei = { 0 };
    worldMousei.x = (int) worldMouse.x;
    worldMousei.y = (int) worldMouse.y;

    Rectangle bg = { 0 };
    bg.width = 200.f;
    bg.height = GetScreenHeight();
    bg.x = GetScreenWidth() - bg.width;
    bg.y = 0;

    if (max.x < width) m_camera.offset.x = width - (max.x - (float)width/2);
    if (max.y < height) m_camera.offset.y = height - (max.y - (float)height/2);
    if (min.x > 0) m_camera.offset.x = (float)width/2 - min.x;
    if (min.y > 0) m_camera.offset.y = (float)height/2 - min.y;
    
    for (int i = 0; i < BLOCKS_COUNT; i++) {
        if (IsKeyPressed(KEY_ONE + i)) {
            if (!m_coll) {
                m_currentBlock = (Block) i; 
            } else {
                m_currentColl = (Collectibles) i;
            }
        }
    }

    if (IsKeyDown(KEY_A) && m_cameraPos.x >= 14) {
        m_cameraPos.x -= 0.2f;
    }

    if (IsKeyDown(KEY_D) && m_cameraPos.x < WORLD_SIZE) {
        m_cameraPos.x += 0.2f;
    }

    if (IsKeyDown(KEY_W)) {
        m_cameraPos.y -= 0.2f;
    }

    if (IsKeyDown(KEY_S)) {
        m_cameraPos.y += 0.2f;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !CheckCollisionPointRec(mousePos, bg)) {
        if (m_level.GetBlock(worldMousei.x, worldMousei.y)) {
            m_level.setBlock(AIR, worldMousei.x, worldMousei.y);
        } else if (m_level.containsCollectible({worldMousei.x, worldMousei.y})) {
            m_level.removeCollectible({worldMousei.x, worldMousei.y});
        } else if (m_level.containsSpawnpoint({worldMousei.x, worldMousei.y})) {
            m_level.removeSpawnpoint({worldMousei.x, worldMousei.y});
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !CheckCollisionPointRec(mousePos, bg)) {
        if (m_spawn) {
            m_level.setSpawnpoint({worldMousei.x, worldMousei.y});
        } else if (m_coll) {
            m_level.addCollectible({{worldMousei.x, worldMousei.y}, (Collectibles)(m_currentColl + 1)});
        } else {
            m_level.setBlock(m_currentBlock, worldMousei.x, worldMousei.y);
        }
    }

    if (IsKeyDown(KEY_LEFT_SHIFT) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        m_level.setBlock(m_currentBlock, worldMousei.x, worldMousei.y);
    }
}

void Game::renderEditor() {
    auto mousePos = GetMousePosition();
    Vector2 worldMouse = GetScreenToWorld2D({std::trunc(mousePos.x), std::trunc(mousePos.y)}, m_camera);

    ClearBackground({4, 4, 50, 255});

    BeginMode2D(m_camera);
        m_level.render();
    EndMode2D();    

    // GUI
    // TODO: Rewrite
    Rectangle bg = { 0 };
    bg.width = 200.f;
    bg.height = GetScreenHeight();
    bg.x = GetScreenWidth() - bg.width;
    bg.y = 0;

    DrawRectangleRec({bg}, {10, 10, 10, 255});

    DrawFPS(0, 0);
    DrawText(TextFormat("%f\n%f", m_cameraPos.x, m_cameraPos.y), 0, 20, 20, WHITE);
    DrawText(TextFormat("%f\n%f", worldMouse.x, worldMouse.y), 0, 60, 20, WHITE);

    float padding = 35.f;

    Rectangle saveButton = { 0 };
    saveButton.width = bg.width - padding;
    saveButton.height = padding;
    saveButton.x = bg.x + (padding / 2);
    saveButton.y = padding;

    if (GuiButton(saveButton, "Save")) {
        m_level.write();
    }

    Rectangle collButton = { 0 };
    collButton.width = bg.width - padding;
    collButton.height = padding;
    collButton.x = bg.x + (padding / 2);
    collButton.y = padding * 2 + 10;

    if (GuiButton(collButton, "Collectibles")) {
        m_coll ^= 1;
        m_spawn = false;
    }

    Rectangle respawnButton = { 0 };
    respawnButton.width = bg.width - padding;
    respawnButton.height = padding;
    respawnButton.x = bg.x + (padding / 2);
    respawnButton.y = padding * 3 + 20;

    if (GuiButton(respawnButton, "Respawn")) {
        m_spawn ^= 1;
        m_coll = false;
    }

    auto fontSize = 20;
    auto text = "Current block:";
    auto blockTextY = respawnButton.y + padding + fontSize;

    DrawText(text, bg.x + fontSize, blockTextY, fontSize, WHITE);
    float blockX = ((m_currentBlock - 1) % 16) * 8.f;
    float blockY = ((m_currentBlock - 1) / 16) * 8.f;

    if (m_currentBlock == 0) {
        DrawText("Air", bg.x + fontSize, blockTextY + fontSize, fontSize, WHITE);
    } else {
        DrawTexturePro(m_blocks, 
            {blockX, blockY, 8.f, 8.f}, 
            {bg.x + 48, blockTextY + padding, 48.f, 48.f},
            {0, 0}, 
            0, WHITE);
    }
    
}