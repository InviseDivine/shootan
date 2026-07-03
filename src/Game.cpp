#include "Types.hpp"
#include "Multiplayer.hpp"
#include <Game.hpp>
#include <cmath>
#include <iostream>
#include <Utils.hpp>
#include <Collisions.hpp>
#include <raymath.h>
#include <thread>

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
    // temp
    for (int i = 0; i < 64; i++) {
        m_level.getWorld().at(PACK_INDEX(i, 64, 256)) = 1;
    }

    InitWindow(1280, 720, "Shootan YOOO");

    SetTargetFPS(60);

    m_player = {nickname, 1, 63, 100, 0, {GUN}, {0, 0}, true};

    auto& mp = Multiplayer::get();
    std::thread(&Multiplayer::init, &mp, m_player.nickname, "localhost", 6890).detach();

    m_camera = { 0 };

    m_camera.offset = Vector2{ GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    m_camera.rotation = 0.0f;
    m_camera.zoom = 50.0f;
    
    m_pistol = LoadTexture("assets/pistol.png");

    while (!WindowShouldClose()) {
        update();

        BeginDrawing();
            render();
        EndDrawing();
    }
}
void Game::update() {
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

    m_level.update();


    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        std::cout << "test" << std::endl;
        
        Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), m_camera);

        Vector2 gunPos = {m_player.x + 0.5f, m_player.y + 0.5f};

        Vector2 direction = Vector2Subtract(mousePos, gunPos);

        float angle = atan2f(direction.y, direction.x);

        auto& mp = Multiplayer::get();

        // m_level.addBullet(Bullet {{gunPos.x, gunPos.y}, {cosf(angle) * 0.1f, sinf(angle) * 0.1f}});
        auto addBulletPacket = new char[HEADER_SIZE + 4];
        addBulletPacket[0] = Header::ADDBULLET;

        *(float*)(addBulletPacket + HEADER_SIZE) = angle;

        mp.sendPacket(addBulletPacket, HEADER_SIZE + 4, true);

        delete [] addBulletPacket;
    }
    
    if ((IsKeyDown(KEY_W) || IsKeyDown(KEY_SPACE) ||  IsKeyDown(KEY_UP)) && m_player.onGround) {
        m_player.speed.y = -0.3f;
    }

    if (IsKeyDown(KEY_A)) {
        m_player.speed.x = -0.1f;
    }

    if (IsKeyDown(KEY_D)) {
        m_player.speed.x = 0.1f;
    }
        
    m_player.speed.y += 0.02f;

    float prevX = m_player.speed.x;
    float prevY = m_player.speed.y;

    Vector2 blocksAroundArr[10] = { 0 };
    
    int blocksAroundCount = 0;
    for (int yy = (int)m_player.y - 1; yy <= (int)(m_player.y + 5.f); yy++) {
        for (int xx = (int)m_player.x - 1; xx <= (int)(m_player.x + 5.f); xx++) {
            if (m_level.GetBlock(xx, yy)) {
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
    m_player.x += x;
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

    if (m_player.speed.x < 0.01f) m_player.speed.x = 0;
    m_player.speed.x *= 0.91f;
    m_player.speed.y *= 0.98f;
}
void Game::render() {
    ClearBackground({4, 4, 50, 255});
    
    Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), m_camera);

    Vector2 origin = GetScreenToWorld2D({45.f, 45.f}, m_camera);

    Vector2 direction = Vector2Subtract(mousePos, {m_player.x + 0.5f, m_player.y + 0.5f});

    float angle = atan2f(direction.y, direction.x) * RAD2DEG;
    
    int flip = angle >= 90 && angle < 270 ? -1 : 1;
    
    DrawFPS(0, 0);
    BeginMode2D(m_camera);
        m_level.render();

        for (auto& [_, client] : m_players) {            
            DrawRectangleRec({client.x, client.y, 1.f, 1.f}, MAROON);   
        }
        
        DrawRectangleRec({m_player.x, m_player.y, 1.f, 1.f}, MAROON);   

        DrawTexturePro(m_pistol, {0, 0, static_cast<float>(m_pistol.width), static_cast<float>(m_pistol.height * flip)},
            {m_player.x + 0.5f, m_player.y + 0.5f, 1.5f, 1.5f}, {0.75f, 0.75f}, angle, WHITE);
    EndMode2D();
}