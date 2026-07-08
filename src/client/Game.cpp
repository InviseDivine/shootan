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
#include "ResourceManager.hpp"

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

    // m_editor = 1;

    SetExitKey(KEY_NULL);

    m_player = {nickname, 1, 61, 100, 0, {true}, {0, 0}, true};

    // TODO: ResourceManager
    auto& rm = ResourceManager::get();
    rm.init();

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

    GuiSetStyle(DEFAULT, TEXT_SIZE, 25);

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

    auto mousePos = GetMousePosition();
    
    m_camera.target = Vector2 {m_player.x, m_player.y};

    if (m_player.currentWeapon == SNIPER_RIFLE) {
        static Vector2 zoom = { 0 };

        Rectangle boxLeft = { 0 };
        boxLeft.width = 200.f;
        boxLeft.height = GetScreenHeight();
        boxLeft.x = GetScreenWidth() - boxLeft.width;
        boxLeft.y = 0;

        Rectangle boxRight = { 0 };
        boxRight.width = 200.f;
        boxRight.height = GetScreenHeight();
        boxRight.x = 0;
        boxRight.y = 0;

        if (CheckCollisionPointRec(mousePos, boxRight) && m_camera.offset.x < width / 2.0f + 200) {
            zoom.x += 10.f;
        } 

        if (CheckCollisionPointRec(mousePos, boxLeft) && m_camera.offset.x > width / 2.0f - 200) {
            zoom.x -= 10.f;
        } 

        m_camera.offset = Vector2 { width / 2.0f + zoom.x, height / 2.0f + zoom.y};  
    } else {
        m_camera.offset = Vector2 { width / 2.0f, height / 2.0f };  
    }
    
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

    if (!m_chatOpened) {
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
    } else {
        if (IsKeyPressed(KEY_ENTER)) {
            std::string msg(m_message);
            auto& mp = Multiplayer::get();

            auto msgSize = HEADER_SIZE + msg.size();
            auto msgPacket = new char[msgSize];

            msgPacket[0] = Header::MESSAGE;

            memcpy(msgPacket + 1, msg.data(), msg.length());

            mp.sendPacket(msgPacket, msgSize, true);

            delete [] msgPacket;

            m_chatOpened ^= 1;

            memset(m_message, 0, sizeof(m_message));
        }

        if (IsKeyPressed(KEY_ESCAPE)) m_chatOpened ^= 1;
    }
    float wheel = GetMouseWheelMove();

    if (wheel != 0.f) {
        auto move = (wheel < 0.f) ? -1 : 1;

        auto updateWeapon = new char[HEADER_SIZE + sizeof(uint8_t)];
        updateWeapon[0] = UPDATEWEAPON;
        updateWeapon[1] = m_player.currentWeapon + move < 0 ? WEAPONS_COUNT - 1 : m_player.currentWeapon + move;
        
        auto& mp = Multiplayer::get();
        
        mp.sendPacket(updateWeapon, 2, true);
        
        delete [] updateWeapon;
    }

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

        if (m_player.speed.x > -0.05f && m_player.speed.x < 0.05f) m_player.speed.x = 0;
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

    int flip = !(angle >= -90 && angle < 90) ? 1 : 0;

    auto& rm = ResourceManager::get();

    BeginMode2D(m_camera);
        m_level.render();
        for (auto& [_, client] : m_players) {  

            auto fontSize = 0.5f;
            auto text = TextFormat("%s %d", client.nickname.c_str(), client.hp);
            float spacing = 0.05f;

            int flipClient = !(client.angle >= -90 && client.angle < 90) ? 1 : 0;

            rm.drawSpriteFromSheet(PLAYER_SPRITE, {client.x, client.y, 1.f, 1.f}, {0, 0}, 0, WHITE, flipClient);

            rm.drawWeaponPlayer((Weapons)client.currentWeapon, client.angle, {client.x, client.y}, WHITE, flipClient);

            DrawTextPro(GetFontDefault(), text, {client.x - (MeasureTextEx(GetFontDefault(), text, fontSize, spacing).x / 4), client.y - 0.55f}, {0, 0}, 0, fontSize, spacing, WHITE);
        }
        rm.drawSpriteFromSheet(PLAYER_SPRITE, {m_player.x, m_player.y, 1.f, 1.f}, {0, 0}, 0, WHITE, flip);

        // DrawRectangleRec({m_player.x, m_player.y, 1.f, 1.f}, MAROON);   
        rm.drawSpriteFromSheet(WIZARD_HAT_SPRITE, {m_player.x, m_player.y - 1.f, 1.f, 1.f}, {0, 0}, 0, WHITE, flip);

        rm.drawWeaponPlayer((Weapons)m_player.currentWeapon, angle, {m_player.x, m_player.y}, WHITE, flip);

    EndMode2D();
    
    // Debug info
    DrawFPS(0, 0);
    DrawText(TextFormat("%f\n%f", m_player.x, m_player.y), 0, 20, 20, WHITE);

    if (m_player.reload > 0) {
        DrawText("Reloading...", 0, 80, 40, WHITE);
    }
    
    Rectangle hpBarBg = { 0 };
    hpBarBg.width = 4 * 100;
    hpBarBg.height = 25;
    hpBarBg.x = GetScreenWidth() - hpBarBg.width;
    hpBarBg.y = 0;

    Rectangle hpBar = { 0 };
    hpBar.width = 4 * m_player.hp;
    hpBar.height = hpBarBg.height;
    hpBar.x = GetScreenWidth() - hpBarBg.width;
    hpBar.y = 0;
    
    Color color = m_player.hp > 60 ? GREEN : m_player.hp > 25 ? ORANGE : RED;
    Color lines = m_player.hp > 60 ? DARKGREEN : m_player.hp > 25 ? Color {128, 78, 4, 255} : Color {107, 4, 4, 255};

    DrawRectangleRec(hpBarBg, BLACK);
    DrawRectangleRec(hpBar, color);
    DrawRectangleLinesEx(hpBarBg, 5, lines);
    auto hpText = TextFormat("HP: %d", m_player.hp);
    
    DrawText(hpText, (hpBarBg.width - MeasureText(hpText, 20)) / 2 + hpBarBg.x, 4, 20, WHITE);
    Rectangle chatBar = { 0 };

    chatBar.width = GetScreenWidth();   
    chatBar.height = 50.f;
    chatBar.x = 0;
    chatBar.y = GetScreenHeight() - chatBar.height;

    if (m_chatOpened) {
        GuiSetStyle(TEXTBOX, BORDER_COLOR_PRESSED, 0xFFFFFFFF);
        GuiSetStyle(TEXTBOX, BASE_COLOR_PRESSED, 0x000000EE);
        GuiSetStyle(TEXTBOX, TEXT_COLOR_PRESSED, 0xFFFFFFFF);

        GuiTextBox(chatBar, m_message, 255, true);
    }

    // It should before GuiTextBox
    if (IsKeyPressed(KEY_T) && !m_chatOpened) {
        m_chatOpened ^= 1;
    }

    auto msgY = GetScreenHeight();

    for (int i = m_messages.size(); i > 0; i--) {
        auto& msg = m_messages.at(i - 1);

        if (m_chatOpened || msg.lifeTime > 0) {
            DrawText(msg.text.c_str(), 0, msgY - 20 - chatBar.height, 20, WHITE);
            
            msgY -= 30;
        }

        if (msg.lifeTime > 0) {
            msg.lifeTime -= 10.f;
        }
    }

    float weaponY = GetScreenHeight();

    for (int i = 0; i < WEAPONS_COUNT; i++) {
        if (m_player.inventory.at(i)) {
            auto sprite = rm.getWeaponSprite((Weapons) i);
            auto& size = rm.getSpriteSize(sprite);
            Vector2 destSize = {size.x * 4, size.y * 4};

            Rectangle src = { 0 };
            src.width = destSize.x;
            src.height = destSize.y;
            src.x = GetScreenWidth() - destSize.x;
            src.y = weaponY - destSize.y;

            Color color = i == m_player.currentWeapon ? WHITE : Color {255, 255, 255, 130};

            rm.drawSpriteFromSheet(sprite, src, {0, 0}, 0, color);

            weaponY -= destSize.y + 10.f;
        }
    }

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
    
    if (m_coll) {
        for (int i = 0; i < COLLECTIBLIES_COUNT; i++) {
            if (IsKeyPressed(KEY_ONE + i)) {
                m_currentColl = (Collectibles) i; 
            }
        }
    } else {
        for (int i = 0; i < BLOCKS_COUNT; i++) {
            if (IsKeyPressed(KEY_ONE + i)) {
                m_currentBlock = (Block) i; 
            }
        }
    }

    if (IsKeyDown(KEY_A) && m_cameraPos.x >= 12) {
        m_cameraPos.x -= 0.2f;
    }

    if (IsKeyDown(KEY_D) && m_cameraPos.x < WORLD_SIZE - 12) {
        m_cameraPos.x += 0.2f;
    }

    if (IsKeyDown(KEY_W)) {
        m_cameraPos.y -= 0.2f;
    }

    if (IsKeyDown(KEY_S) && m_cameraPos.y < WORLD_SIZE - 7) {
        m_cameraPos.y += 0.2f;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !CheckCollisionPointRec(mousePos, bg)) {
        if (m_level.GetBlock(worldMousei.x, worldMousei.y)) {
            m_level.setBlock(AIR, worldMousei.x, worldMousei.y);
        } else if (m_level.containsCollectible({worldMousei.x, worldMousei.y})) {
            m_level.removeCollectible({worldMousei.x, worldMousei.y});
        } else if (m_level.GetBackgroundBlock(worldMousei.x, worldMousei.y)) {
            m_level.setBackgroundBlock(AIR, worldMousei.x, worldMousei.y);
        } else if (m_level.containsSpawnpoint({worldMousei.x, worldMousei.y})) {
            m_level.removeSpawnpoint({worldMousei.x, worldMousei.y});
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !CheckCollisionPointRec(mousePos, bg)) {
        if (m_spawn) {
            m_level.setSpawnpoint({worldMousei.x, worldMousei.y});
        } else if (m_coll) {
            m_level.addCollectible({{worldMousei.x, worldMousei.y}, (Collectibles)(m_currentColl + 1)});
        } else if (IsKeyDown(KEY_Q)) {
            m_level.setBackgroundBlock(m_currentBlock, worldMousei.x, worldMousei.y);
        } else {
            m_level.setBlock(m_currentBlock, worldMousei.x, worldMousei.y);
        }
    }

    if (IsKeyDown(KEY_LEFT_SHIFT) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (IsKeyDown(KEY_Q)) {
            m_level.setBackgroundBlock(m_currentBlock, worldMousei.x, worldMousei.y);
        } else {
            m_level.setBlock(m_currentBlock, worldMousei.x, worldMousei.y);
        }
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

    auto pad = m_currentBlock ? 48.f : 0;

    float modeTextY = blockTextY + padding + fontSize + pad;

    auto currMode = m_coll ? "Collectibles" : m_spawn ? "Spawnpoints" : "Blocks";

    DrawText(TextFormat("Current mode:\n%s", currMode), bg.x + fontSize, modeTextY, fontSize, WHITE);

    auto& rm = ResourceManager::get();

    if (m_currentBlock == 0) {
        DrawText("Air", bg.x + fontSize, blockTextY + fontSize, fontSize, WHITE);
    } else {
        rm.drawBlock((Block)(m_currentBlock - 1), {bg.x + 48, blockTextY + padding, 48.f, 48.f}, WHITE);
    }
}