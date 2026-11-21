#include "Types.hpp"
#include "Multiplayer.hpp"
#include <Game.hpp>
#include <cmath>
#include <Collisions.hpp>
#include <raylib.h>
#include <raymath.h>
#include <thread>
#include <Weapons.hpp>
#include <raygui.h>
#include "ResourceManager.hpp"
#include <cstring>
#include <MenuScene.hpp>
#include <algorithm>
#include <fstream>

std::string Game::getSetting(std::string key) {
    std::ifstream config("config.txt");
    std::string line;

    if (config.is_open()) {
        while (std::getline(config, line)) {
            if (line.starts_with(key)) {
                auto toFind = key + "=";

                auto value = line.substr(toFind.size(), line.size() - toFind.size());

                return value;
            }
        }
    }

    return "";
}

void Game::setEnd(bool end, uint32_t id) {
    m_end = end;
    if (id == 0) {
        for (auto& [id, plr] : m_players) {
            plr.currentWeapon = (WeaponType) 0;
            plr.hp = 100;
            plr.score = 0;
        }

        m_player.hp = 100;
        m_player.score = 0;
        m_player.currentWeapon = (WeaponType) 0;
        m_player.inventory = {true};
        
        m_winner = 0;
    } else {
        m_winner = id;
    }
}

void Game::drawScore() {
    auto& rm = ResourceManager::get();

    float x = 75;
    float y = 75;

    float width = GetScreenWidth() - x * 2;
    float height = GetScreenHeight() - y * 2;
    
    Rectangle bg = {x, y, width, height};

    DrawRectangleRec(bg, {0, 0, 0, 180});
    DrawText("Score", width - x - 10, y + 10, 40, WHITE);

    static std::vector<std::pair<uint32_t, Player*>> players;
    {
        players.clear();
        players.push_back(std::pair(m_myId, &m_player));

        for (auto& [id, plr] : m_players) {
            players.push_back(std::pair(id, &plr));
        }

        if (players.size() > 1) {
            std::sort(players.begin(), players.end(), [](std::pair<uint32_t, Player*> a, std::pair<uint32_t, Player*> b) {
                return a.second->score > b.second->score;
            });
        }
    }

    auto count = 0;
    for (auto& [id, client] : players) {
        float yClient = y + 20 * count + 60;
        auto text = TextFormat("%s - %d", client->nickname.c_str(), client->score);

        DrawText(text, x + 40, yClient, 20, WHITE);

        if (m_winner == id) {
            rm.drawSpriteFromSheet(CROWN_SPRITE, {x + 40 + MeasureText(text, 20) + 20, yClient, 20.f, 20.f}, {0, 0}, 0, WHITE);
        }
        count++;
    }
}
void Game::setMyHp(int hp) {
    m_player.hp = hp;
}

void Game::cleanup() {
    m_level = Level();
    auto hat = m_player.hat;
    m_player = Player {
        .x = 0, 
        .y = 0, 
        .hp = 100, 
        .nickname = m_player.nickname,
        .currentWeapon = (WeaponType) 0, 
        .inventory = {true}
    };
    m_player.hat = hat;
    m_messages.clear();
    m_players.clear();
    m_myId = 0;
    m_chatOpened = false;
    memset(m_message, 0, sizeof(m_message));
}

void Game::startMpThread(std::string srv) {
    // TODO: Connecting screen
    cleanup();
    
    std::stringstream ss(srv);
    
    std::vector<std::string> result;
    std::string token;

    while (std::getline(ss, token, ':')) {
        result.push_back(token);
    }
    
    std::string ip = result.at(0);
    int port = result.size() > 1 ? std::stoi(result.at(1)) : 6890;
    m_player.nickname.resize(strlen(m_player.nickname.c_str()));

    auto& mp = Multiplayer::get();

    m_lastServer = srv;
    m_lastPort = port;

    std::thread(&Multiplayer::init, &mp, m_player.nickname, ip, port).detach();
}

void Game::updatePlayer() {
    if (!m_player.isDied) {
        if (!m_player.onLadder) {
            m_player.speed.y += 0.02f;
        }

        float prevX = m_player.speed.x;
        float prevY = m_player.speed.y;

        int blocksAroundCount = 10;
        
        std::vector<Vector2> blocksAroundArr = m_level.getBlocksAround({m_player.x, m_player.y}, 3);
        
        Rectangle playerBox = {m_player.x, m_player.y, 1.f, 1.f};

        float x = m_player.speed.x;
        // Check for X collision
        for (int i = 0; i < blocksAroundArr.size(); i++) {
            x = ClipX(Rectangle{blocksAroundArr.at(i).x, blocksAroundArr.at(i).y, 1.0f, 1.0f}, playerBox, x);
        }
        auto tempX = m_player.x + x;

        if (tempX >= 0 && tempX < WORLD_SIZE - 1) {
            playerBox.x = m_player.x = tempX;
        }

        // printf("%f \n", x);
        float y = m_player.speed.y;
        // Check for Y collision
        for (int i = 0; i < blocksAroundArr.size(); i++) {
            y = ClipY(Rectangle{blocksAroundArr.at(i).x, blocksAroundArr.at(i).y, 1.0f, 1.0f}, playerBox, y);
        }
        m_player.y += y;
        // printf("%f \n", y);

        m_player.onGround = prevY != y && prevY > 0.f;

        if (m_player.onGround) {
            m_coyotTime = 60.f;
        } else {
            m_coyotTime -= 10.f;
        }

        // Stop motion on collision
        if (prevX != x) m_player.speed.x = 0.f;
        if (prevY != y) m_player.speed.y = 0.f;

        if (m_player.speed.x != 0 || m_player.speed.y != 0) sendMovePacket();

        if (m_player.speed.x > -0.01f && m_player.speed.x < 0.01f) m_player.speed.x = 0;

        if (!m_player.onLadder) {
            m_player.speed.x *= 0.89f;
            m_player.speed.y *= 0.98f;
        } else {
            m_player.speed.x = 0;
            m_player.speed.y = 0;   
        }
    }
}

void Game::addNotification(uint32_t id, int damage) {
    auto plr = id == m_myId ? m_player : m_players.at(id);
    Vector2 pos = GetWorldToScreen2D(Vector2 { plr.x + 1.f, plr.y + 0.5f }, m_camera);

    m_killsNotifications.push_back(Message {std::format("-{}", damage), 0, 255, {pos.x, pos.y}});
}

void Game::addNotification(int score) {
    m_player.score = score;

    float x = std::rand() % (GetScreenWidth() - 40); 
    float y = std::rand() % (GetScreenHeight() - 40);

    m_killsNotifications.push_back(Message {"+1 Kill", 0, 255, {x, y}});
}

void Game::sendMovePacket() {
    if (!m_player.isDied) {
        MovePacket movePacket {
            .position = Vector2 {m_player.x, m_player.y}
        };

        auto& mp = Multiplayer::get();
        
        mp.sendPacket((char*)&movePacket, sizeof(movePacket), false);
    }
}

void Game::detectInput() {
    if (GetKeyPressed() > 0 || IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        m_currentInput = InputType::KEYBOARD;
    }

    if (IsGamepadAvailable(0)) {
        float leftStickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        float leftStickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);

        float rightStickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
        float rightStickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);

        float rightTrigger = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_TRIGGER);

        if (rightTrigger < -0.9f) rightTrigger = -1.0f;

        if (leftStickX > -0.1f && leftStickX < 0.1f) leftStickX = 0.0f;
        if (leftStickY > -0.1f && leftStickY < 0.1f) leftStickY = 0.0f;

        if (rightStickX > -0.1f && rightStickX < 0.1f) rightStickX = 0.0f;
        if (rightStickY > -0.1f && rightStickY < 0.1f) rightStickY = 0.0f;

        if (rightTrigger != -1.0f) m_currentInput = InputType::GAMEPAD;
        if (leftStickX != 0.0f) m_currentInput = InputType::GAMEPAD;
        if (rightStickX != 0.0f) m_currentInput = InputType::GAMEPAD;
        if (leftStickY != 0.0f) m_currentInput = InputType::GAMEPAD;
        if (rightStickY != 0.0f) m_currentInput = InputType::GAMEPAD;
    }
}

void Game::init() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);

    InitWindow(1280, 720, "Shootan");
    
    // Parsing settings
    std::ifstream config("config.txt");
    if (config.is_open()) {
        auto server = getSetting("server");
        if (server != "") {
            m_lastServer = server;
        }

        auto nickname = getSetting("nickname");
        if (nickname != "") {
            m_player.nickname = nickname;
        }

        auto hat = getSetting("hat");
        if (hat != "") {
            m_player.hat = (Hat)std::stoi(hat);
        }

        config.close();
    } else {
        std::ofstream configWrite("config.txt");
        auto nickname = std::format("Player-{}", rand() % 1000);

        if (configWrite.is_open()) {
            configWrite << "server=sffempire.ru" << std::endl;
            configWrite << "nickname=" << nickname << std::endl;
            configWrite << "hat=0";
        }
        
        m_player.nickname = nickname;
        m_player.hat = Hat::NONE_HAT;
        m_lastServer = "sffempire.ru";

        configWrite.close();
    }

    SetTargetFPS(60);
#ifdef __linux__
    if(getenv("SteamDeck")) ToggleFullscreen();
#endif

    SetExitKey(KEY_NULL);

    Image icon = LoadImage("assets/ico.png");    
    SetWindowIcon(icon);
    UnloadImage(icon);    

    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
    HideCursor();

    auto& rm = ResourceManager::get();
    rm.init();

    m_scene = std::make_shared<MenuScene>();

    m_camera = { 0 };

    m_camera.offset = Vector2 { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    m_camera.rotation = 0.0f;
    m_camera.zoom = 50.0f;
    m_cameraPos = {64, 120};

    while (!WindowShouldClose()) {
        detectInput();

        // Move cursor
        if (m_currentInput == InputType::GAMEPAD) {
            float rightStickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
            float rightStickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);
            float leftStickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
            float leftStickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);

            if (leftStickX > -0.1f && leftStickX < 0.1f) leftStickX = 0.0f;
            if (leftStickY > -0.1f && leftStickY < 0.1f) leftStickY = 0.0f;

            if (rightStickX > -0.1f && rightStickX < 0.1f) rightStickX = 0.0f;
            if (rightStickY > -0.1f && rightStickY < 0.1f) rightStickY = 0.0f;

            if (m_gameplayCursor) {
                auto playerCenter = GetWorldToScreen2D({m_player.x + 0.5f, m_player.y + 0.5f}, m_camera);

                if (rightStickY == 0 && rightStickX == 0) {
                    m_cursorPos = {playerCenter.x + 3.f * (leftStickX < 0.f ? -1.f : 1.f), playerCenter.y};
                } else {
                    m_cursorPos = {
                        playerCenter.x + 200.f * rightStickX,
                        playerCenter.y + 200.f * rightStickY
                    };
                }
            } else {
                if (leftStickX > 0) m_cursorPos.x += 5;
                if (leftStickX < 0) m_cursorPos.x -= 5;

                if (leftStickY < 0) m_cursorPos.y -= 5;
                if (leftStickY > 0) m_cursorPos.y += 5;
            }
        } else if (m_currentInput == InputType::KEYBOARD) {
            m_cursorPos = GetMousePosition();
        }

        if (m_scene != nullptr) {
            m_scene->update();
        } else {
            if (m_editor) {
                updateEditor();
            } else {
                update();
            }
        }

        BeginDrawing();
            auto& rm = ResourceManager::get();

            if (m_scene != nullptr) {
                ClearBackground(m_scene->getColor());

                m_scene->render();
            } else {
                if (m_editor) {
                    renderEditor();
                } else {
                    render();
                }
            }

            rm.drawSpriteFromSheet(CURSOR_SPRITE, {m_cursorPos.x - (int)(15.f / 2), m_cursorPos.y - (int)(15.f / 2), 15.f, 15.f});
        EndDrawing();

        if (m_cleanScene) {
            m_scene = nullptr;
            m_cleanScene = false;
        }
    }

    std::ofstream configWrite("config.txt");
    
    if (configWrite.is_open()) {
        configWrite << "server=" << m_lastServer << std::endl;
        configWrite << "nickname=" << m_player.nickname << std::endl;
        configWrite << "hat=" << (int)(m_player.hat);
        
        configWrite.close();
    }

    CloseWindow();
}

// --------------------
//      Main game
// --------------------
void Game::update() {
    m_timer.advanceTime();

    m_gameplayCursor = m_paused ? false : true; 
    float zoomWidth = (float) GetScreenWidth() / 25.6;
    float zoomHeight = (float) GetScreenHeight() / 14.4;

    m_camera.zoom = fmaxf(zoomWidth, zoomHeight);

    auto width = GetScreenWidth();
    auto height = GetScreenHeight();
    
    m_camera.target = Vector2 {m_player.x, m_player.y};

    Vector2 worldMousePos =  GetScreenToWorld2D(m_cursorPos, m_camera);
    static Vector2 zoom = { 0 };

    // TODO: Add up and down zoom
    if (m_player.currentWeapon == SNIPER_RIFLE) {
        if (m_currentInput == InputType::GAMEPAD && IsGamepadAvailable(0)) {
            float rightStickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
            float rightStickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);

            if (rightStickX > -0.1f && rightStickX < 0.1f) rightStickX = 0.0f;
            if (rightStickY > -0.1f && rightStickY < 0.1f) rightStickY = 0.0f;  

            if (rightStickX < 0 && m_camera.offset.x < width / 2.0f + 300) {
                zoom.x += 10.f;
            }

            if (rightStickX > 0 && m_camera.offset.x > width / 2.0f - 300) {
                zoom.x -= 10.f;
            }
        } else {
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

            if (CheckCollisionPointRec(GetMousePosition(), boxRight)) {
                if (m_camera.offset.x < width / 2.0f + 550) {
                    zoom.x += 15.f;
                } else {
                    zoom.x = 550.f;
                }
            } 
            
            if (CheckCollisionPointRec(GetMousePosition(), boxLeft)) {
                if (m_camera.offset.x > width / 2.0f - 600) {
                    zoom.x -= 15.f;
                } else {
                    zoom.x = -600.f;
                }
            }      
        }
        m_camera.offset = Vector2 { width / 2.0f + zoom.x, height / 2.0f + zoom.y};  
    } else {
        m_camera.offset = Vector2 { width / 2.0f, height / 2.0f };  
        zoom = {0, 0};
    }
    
    Vector2 max = GetWorldToScreen2D(Vector2 { WORLD_SIZE, WORLD_SIZE }, m_camera);
    Vector2 min = GetWorldToScreen2D(Vector2 { 0, 0 }, m_camera);

    if (max.x < width) m_camera.offset.x = width - (max.x - (float)width/2);
    if (min.x > 0) m_camera.offset.x = (float)width/2 - min.x;
    
    if (max.y < height) m_camera.offset.y = height - (max.y - (float)height/2);
    if (min.y > 0) m_camera.offset.y = (float)height/2 - min.y;

    for (uint32_t i = 0; i < m_timer.getTicks(); i++) {
        m_level.update();
        
        for (int i = 0; i < m_player.reload.size(); i++) {
            auto& reload = m_player.reload.at(i);

            if (reload > 0) {
                reload -= 1.f;
            }
        }
    }

    if (!m_chatOpened && !m_paused) {
        if (IsKeyPressed(KEY_F3)) {
            m_showDebug ^= 1;
        }

        if (m_player.isDied && IsKeyPressed(KEY_R)) {
            RevivePacket packet {};
            auto& mp = Multiplayer::get();

            mp.sendPacket((char*)&packet, sizeof(RevivePacket), true);
        }

        // TODO: Rewrite
        if (IsGamepadAvailable(0) && m_currentInput == InputType::GAMEPAD) { 
            float leftStickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
            float leftStickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);

            float rightStickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
            float rightStickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);

            float rightTrigger = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_TRIGGER);

            if (rightTrigger < -0.9f) rightTrigger = -1.0f;

            if (leftStickX > -0.1f && leftStickX < 0.1f) leftStickX = 0.0f;
            if (leftStickY > -0.1f && leftStickY < 0.1f) leftStickY = 0.0f;

            if (rightStickX > -0.1f && rightStickX < 0.1f) rightStickX = 0.0f;
            if (rightStickY > -0.1f && rightStickY < 0.1f) rightStickY = 0.0f;
            
            if (!m_player.onLadder) {
                if ((m_level.GetBlock(std::ceil(m_player.x), std::ceil(m_player.y)) == LADDER ||
                    m_level.GetBlock(std::floor(m_player.x), std::floor(m_player.y)) == LADDER) 
                && leftStickY != 0) {
                    int playerX = m_level.GetBlock(std::ceil(m_player.x), std::ceil(m_player.y)) == LADDER ? std::ceil(m_player.x) :
                    m_level.GetBlock(std::floor(m_player.x), std::floor(m_player.y)) == LADDER ? std::floor(m_player.x) : 0;
                    m_player.onLadder = true;
                    m_player.x = std::trunc(playerX);
                    m_player.speed = {0, 0};
                }
            }
            
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) {
                if (m_coyotTime > 0.f) {
                    m_player.speed.y = -0.3f;
                    m_coyotTime = 0.f;
                } else if (m_player.onLadder) {
                    m_player.onLadder = false;
                    m_player.speed.y = -0.3f;
                }
            }

            // TODO: Rewrite gun angle
            if (rightTrigger > 0) {
                auto& rm = ResourceManager::get();
        
                Vector2 gunPos = {m_player.x + 0.5f, m_player.y + 0.5f};

                Vector2 direction = Vector2Subtract(worldMousePos, gunPos);

                float angle = atan2f(direction.y, direction.x);

                auto& mp = Multiplayer::get();

                // NOTE: Client packet smaller than packet that server send
                ShootPacket bulletPacket {
                    .angle = angle
                };

                mp.sendPacket((char*)&bulletPacket, sizeof(bulletPacket), true);

                m_player.reload.at(m_player.currentWeapon) = weapons.at(m_player.currentWeapon).reloadTime;
            }

            // TODO: Rewrite
            if (m_player.onLadder) {
            // if (m_player.onLadder && IsKeyDown(KEY_W) && m_level.GetBlock(std::trunc(m_player.x), std::ceil(m_player.y - 1)) == LADDER) {
            //     int playerX = m_level.GetBlock(std::ceil(m_player.x), std::ceil(m_player.y)) == LADDER ? std::ceil(m_player.x) :
            //     m_level.GetBlock(std::floor(m_player.x), std::floor(m_player.y)) == LADDER ? std::floor(m_player.x) : 0;

            //     m_player.speed.y = -0.2f;
            //     m_player.x = playerX;
            // }
            // if (m_player.onLadder && (IsKeyDown(KEY_S)) && m_level.GetBlock(std::trunc(m_player.x), std::floor(m_player.y + 1)) == LADDER) {

                if (leftStickY > 0.5f && m_level.GetBlock(std::trunc(m_player.x), std::floor(m_player.y + 1)) == LADDER) {
                    int playerX = m_level.GetBlock(std::ceil(m_player.x), std::ceil(m_player.y)) == LADDER ? std::ceil(m_player.x) :
                    m_level.GetBlock(std::floor(m_player.x), std::floor(m_player.y)) == LADDER ? std::floor(m_player.x) : 0;

                    m_player.speed.y = 0.2f;
                    m_player.x = playerX;
                }

                if (leftStickY < -0.5f && m_level.GetBlock(std::trunc(m_player.x), std::ceil(m_player.y - 1)) == LADDER) {
                    int playerX = m_level.GetBlock(std::ceil(m_player.x), std::ceil(m_player.y)) == LADDER ? std::ceil(m_player.x) :
                    m_level.GetBlock(std::floor(m_player.x), std::floor(m_player.y)) == LADDER ? std::floor(m_player.x) : 0;

                    m_player.speed.y = -0.2f;
                    m_player.x = playerX;
                }
            } else {
                if (leftStickX > 0) {
                    if (m_player.speed.x <= 0.250f) {
                        m_player.speed.x += 0.05f;
                    } else {
                        m_player.speed.x = 0.250f;
                    }
                }

                if (leftStickX < 0) {
                    if (m_player.speed.x >= -0.250f) {
                        m_player.speed.x -= 0.05f;
                    } else {
                        m_player.speed.x = -0.250f;
                    }
                }
            }
            
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1) && m_player.grenade != GRENADE_NONE) {
                Vector2 gunPos = {m_player.x + 0.5f, m_player.y + 0.5f};

                Vector2 direction = Vector2Subtract(worldMousePos, gunPos);
                auto& mp = Multiplayer::get();

                float angle = atan2f(direction.y, direction.x);

                ThrowGrenadePacket throwPacket {
                    .angle = angle
                };
                
                mp.sendPacket((char*)&throwPacket, sizeof(throwPacket), true);

                m_player.grenade = GRENADE_NONE;
            }

            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT)) m_paused ^= 1;

            // Weapons
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) {
                auto& mp = Multiplayer::get();

                UpdateWeaponPacket updatePacket {
                    .index = WeaponType::GUN
                };
                mp.sendPacket((char*)&updatePacket, sizeof(updatePacket), true);
            }

            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) {
                auto& mp = Multiplayer::get();

                UpdateWeaponPacket updatePacket {
                    .index = WeaponType::SHOTGUN
                };
                mp.sendPacket((char*)&updatePacket, sizeof(updatePacket), true);
            }
    
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) {
                auto& mp = Multiplayer::get();

                UpdateWeaponPacket updatePacket {
                    .index = WeaponType::SNIPER_RIFLE
                };
                mp.sendPacket((char*)&updatePacket, sizeof(updatePacket), true);
            }
        }

        if (m_currentInput == InputType::KEYBOARD) {
            if (!m_player.onLadder) {
                bool ladderInRight = m_level.GetBlock(std::floor(m_player.x), std::floor(m_player.y)) == LADDER;
                bool ladderInLeft = m_level.GetBlock(std::ceil(m_player.x), std::ceil(m_player.y)) == LADDER;

                if ((ladderInRight || ladderInLeft) && (IsKeyDown(KEY_W) || IsKeyPressed(KEY_S))) {
                    int playerX = ladderInLeft ? std::ceil(m_player.x) : std::floor(m_player.x);
                    m_player.onLadder = true;
                    m_player.x = playerX;
                    m_player.speed = {0, 0};
                }
            }

            for (uint8_t i = 0; i < m_player.inventory.size(); i++) {
                if (IsKeyPressed(KEY_ONE + i)) {
                    auto& mp = Multiplayer::get();
                    UpdateWeaponPacket updatePacket {
                        .index = (WeaponType) i
                    };
                    mp.sendPacket((char*)&updatePacket, sizeof(updatePacket), true);
                }
            }


            // TODO: Rewrite gun angle
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {  
                auto& rm = ResourceManager::get();
                auto& mp = Multiplayer::get();

                Vector2 gunPos = {m_player.x + 0.5f, m_player.y + 0.5f};
                Vector2 direction = Vector2Subtract(worldMousePos, gunPos);
                float angle = atan2f(direction.y, direction.x);

                ShootPacket updatePacket {
                    .angle = angle
                };
                mp.sendPacket((char*)&updatePacket, sizeof(updatePacket), true);

                m_player.reload.at(m_player.currentWeapon) = weapons.at(m_player.currentWeapon).reloadTime;
            }

            if (IsKeyDown(KEY_SPACE)) {
                if (m_coyotTime > 0.f) {
                    m_player.speed.y = -0.3f;
                    m_coyotTime = 0.f;
                } else if (m_player.onLadder) {
                    m_player.onLadder = false;
                    m_player.speed.y = -0.3f;
                }
            }
            
            bool ladderOnTop = m_level.GetBlock(std::trunc(m_player.x), std::ceil(m_player.y - 1)) == LADDER;
            if (m_player.onLadder && IsKeyDown(KEY_W) && ladderOnTop) {
                m_player.speed.y = -0.2f;
            }

            bool ladderinBottom = m_level.GetBlock(std::trunc(m_player.x), std::floor(m_player.y + 1)) == LADDER;
            if (m_player.onLadder && IsKeyDown(KEY_S) && ladderinBottom) {
                m_player.speed.y = 0.2f;
            }

            if (!m_player.onLadder) {
                if (IsKeyDown(KEY_A)) {
                    if (m_player.speed.x > -0.175f) {
                        m_player.speed.x -= 0.03f;
                    }

                    if (m_player.speed.x < -0.175f) {
                        m_player.speed.x = -0.175f;
                    }
                }

                if (IsKeyDown(KEY_D)) {
                    if (m_player.speed.x < 0.175f) {
                        m_player.speed.x += 0.03f;
                    }

                    if (m_player.speed.x > 0.175f) {
                        m_player.speed.x = 0.175f;
                    }
                }
            }

            if (IsKeyPressed(KEY_F) && m_player.grenade != GRENADE_NONE && !m_player.isDied) {
                Vector2 gunPos = {m_player.x + 0.5f, m_player.y + 0.5f};

                Vector2 direction = Vector2Subtract(worldMousePos, gunPos);
                auto& mp = Multiplayer::get();

                float angle = atan2f(direction.y, direction.x);

                ThrowGrenadePacket updatePacket {
                    .angle = angle
                };
                mp.sendPacket((char*)&updatePacket, sizeof(updatePacket), true);

                m_player.grenade = GRENADE_NONE;
            }   
        }

        float wheel = GetMouseWheelMove();

        if (wheel != 0.f) {
            auto move = (wheel < 0.f) ? -1 : 1;
            auto& mp = Multiplayer::get();

            UpdateWeaponPacket updatePacket {
                .index = WeaponType (m_player.currentWeapon + move < 0 ? WEAPONS_COUNT - 1 : m_player.currentWeapon + move)
            };
            
            mp.sendPacket((char*)&updatePacket, sizeof(updatePacket), true);
        }

        if (IsKeyPressed(KEY_ESCAPE) && !m_chatOpened) {
            m_paused ^= 1;
        }
    } else {
        if (m_currentInput == InputType::KEYBOARD) {
            if (IsKeyPressed(KEY_ENTER)) {
                auto& mp = Multiplayer::get();
                MessagePacket msgPacket {};
                memcpy(msgPacket.message, m_message, 512);
                
                mp.sendPacket((char*)&msgPacket, sizeof(msgPacket), true);

                m_chatOpened ^= 1;

                memset(m_message, 0, sizeof(m_message));
            }

            if (IsKeyPressed(KEY_ESCAPE) && !m_paused) m_chatOpened ^= 1;
        }
    }

    if (m_loaded) {
        updatePlayer();
    }
}
void Game::render() {
    ClearBackground({4, 4, 50, 255});
    
    float zoomWidth = (float) GetScreenWidth() / 1280;
    float zoomHeight = (float) GetScreenHeight() / 720;

    float scale = fmaxf(zoomWidth, zoomHeight);
    
    Vector2 worldMousePos = GetScreenToWorld2D(m_cursorPos, m_camera);

    Vector2 origin = GetScreenToWorld2D({45.f, 45.f}, m_camera);

    Vector2 direction = Vector2Subtract(worldMousePos, {m_player.x + 0.5f, m_player.y + 0.5f});

    float anglePrev = 0;
    float angle = atan2f(direction.y, direction.x) * RAD2DEG;
    
    if (anglePrev != angle && !m_paused) {
        anglePrev = angle;
        
        auto& mp = Multiplayer::get();
        UpdateAnglePacket anglePacket {
            .angle = angle
        };

        mp.sendPacket((char*)&anglePacket, sizeof(anglePacket), true);
    }

    int flip = !(angle >= -90 && angle < 90) ? 1 : 0;

    auto& rm = ResourceManager::get();

    BeginMode2D(m_camera);
        m_level.render();

        // Drawing players
        for (auto& [_, client] : m_players) {  
            if (!client.isDied) {
                auto fontSize = 0.5f;
                Color color = client.hp > 60 ? GREEN : client.hp > 25 ? ORANGE : RED;

                auto nickname = TextFormat("%s", client.nickname.c_str());
                auto hp = TextFormat(" %d", client.hp);

                float spacing = 0.05f;

                int flipClient = !(client.angle >= -90 && client.angle < 90) ? 1 : 0;

                rm.drawSpriteFromSheet(PLAYER_SPRITE, {client.x, client.y, 1.f, 1.f}, {0, 0}, 0, WHITE, flipClient);

                if (m_player.hat != NONE_HAT) {
                    auto& hatPos = rm.getHatPos(client.hat);
                    auto& hatSize = rm.getSpriteSize(rm.getHatSprite(client.hat));
                        
                    rm.drawSpriteFromSheet(
                        rm.getHatSprite(client.hat), 
                        {client.x + hatPos.x / 8, 
                        client.y + hatPos.y / 8, 
                        hatSize.x / 8.f, 
                        hatSize.y / 8.f},
                        {0, 0}, 
                        0, 
                        WHITE, 
                        flipClient
                    );
                }

                rm.drawWeaponPlayer((WeaponType)client.currentWeapon, client.angle, {client.x, client.y}, WHITE, flipClient);
                
                DrawTextPro(GetFontDefault(), 
                    nickname, 
                    {client.x - (MeasureTextEx(GetFontDefault(), nickname, fontSize, spacing).x / 4),
                    client.y - 0.55f},
                    {0, 0}, 0, fontSize, spacing, WHITE
                );

                DrawTextPro(GetFontDefault(), 
                hp, 
                {client.x + MeasureTextEx(GetFontDefault(), nickname, fontSize, spacing).x / 1.5f + 0.25f,
                    client.y - 0.55f},
                {0, 0}, 0, fontSize, spacing, color);
            }
        }
        
        // Current player
        if (!m_player.isDied) {
            rm.drawSpriteFromSheet(PLAYER_SPRITE, {m_player.x, m_player.y, 1.f, 1.f}, {0, 0}, 0, WHITE, flip);

            if (m_player.hat != NONE_HAT) {
                auto& hatPos = rm.getHatPos(m_player.hat);
                auto& hatSize = rm.getSpriteSize(rm.getHatSprite(m_player.hat));

                rm.drawSpriteFromSheet(rm.getHatSprite(m_player.hat), {m_player.x + hatPos.x / 8, m_player.y + hatPos.y / 8, hatSize.x / 8.f, hatSize.y / 8.f},
                {0, 0}, 0, WHITE, flip);
            }

            rm.drawWeaponPlayer((WeaponType)m_player.currentWeapon, angle, {m_player.x, m_player.y}, WHITE, flip);
        }

    EndMode2D();
    
    // Debug info
    if (m_showDebug) {
        DrawText(TextFormat("FPS: %d", GetFPS()), 0, 0, 20, WHITE);
        DrawText(TextFormat("X: %f\nY: %f", m_player.x, m_player.y), 0, 20, 20, WHITE);
    }

    if (m_player.reload.at(m_player.currentWeapon) > 0 && !m_player.isDied) {
        DrawText("Reloading...", 0, 80, 40, WHITE);
    }

    // HP Bar
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

    // Chat bar
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

    // It should be before GuiTextBox
    if (IsKeyPressed(KEY_T) && !m_chatOpened) {
        m_chatOpened ^= 1;
    }

    auto msgY = GetScreenHeight();

    for (int i = m_messages.size(); i > 0; i--) {
        auto& msg = m_messages.at(i - 1);

        if (m_chatOpened || msg.lifeTime > 0) {
            DrawText(msg.text.c_str(), 0, msgY - 20 - chatBar.height, 20, {255, 255, 255, msg.alpha});
            
            msgY -= 30;
        }

        if (msg.lifeTime > 0) {
            msg.lifeTime -= 10.f;
        }
    }

    // Drawing equipement
    float weaponY = GetScreenHeight();
    if (m_player.grenade != GRENADE_NONE) {
        auto size = rm.getSpriteSize(GRENADE_SPRITE);
        Vector2 destSize = {size.x * 4, size.y * 4};

        Rectangle dest = { 0 };
        dest.width = destSize.x;
        dest.height = destSize.y;
        dest.x = GetScreenWidth() - 120;
        dest.y = GetScreenHeight() - destSize.y;

        rm.drawSpriteFromSheet(GRENADE_SPRITE, dest, {0, 0}, 0, WHITE);
    }

    for (int i = 0; i < WEAPONS_COUNT; i++) {
        if (m_player.inventory.at(i)) {
            auto sprite = rm.getWeaponSprite((WeaponType) i);
            auto size = rm.getSpriteSize(sprite);

            Vector2 destSize = {size.x * 4 * scale, size.y * 4 * scale};

            Rectangle dest = { 0 };
            dest.width = destSize.x;
            dest.height = destSize.y;
            dest.x = GetScreenWidth() - destSize.x;
            dest.y = weaponY - destSize.y;

            Color color = i == m_player.currentWeapon ? WHITE : Color {255, 255, 255, 130};

            rm.drawSpriteFromSheet(sprite, dest, {0, 0}, 0, color);

            weaponY -= destSize.y + 10.f;
        }
    }

    // Kill notifications
    for (int i = 0; i < m_killsNotifications.size(); i++) {
        auto& notf = m_killsNotifications.at(i);

        if (notf.alpha > 0) {
            DrawText(notf.text.c_str(), notf.pos.x, notf.pos.y, 20, {255, 255, 255, notf.alpha});

            notf.alpha -= 5;
            notf.pos.y -= 1.f;
        } else {
            m_killsNotifications.erase(m_killsNotifications.begin() + i);
        }
    }

    if (m_died) {
        if (m_alpha > 0) {
            m_alpha -= 5;
        } else {
            m_alpha = 0;
        }

        if (m_diedTicks > 0) {
            m_diedTicks -= 0.5f;

            auto fontSize = 30;
            auto text = "You died!";
            auto width = MeasureText(text, fontSize);
            DrawText(text, (GetScreenWidth() - width) / 2, GetScreenHeight() / 2 - 30 * 3, fontSize, {255, m_alpha, m_alpha, m_alpha});
        } else {
            m_died = false;
        }
    }
    
    if (m_player.hp < 25) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {255, 0, 0, 15});
    }

    if (IsGamepadAvailable(0) && IsGamepadButtonDown(0, GAMEPAD_BUTTON_MIDDLE_LEFT)) {
        drawScore();
    }
    
    // Death screen
    if (m_player.isDied && !m_paused) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, 150});

        auto text = "You died! Press R to respawn";

        DrawText(text, (GetScreenWidth() - MeasureText(text, 30)) / 2, (GetScreenHeight() - 30) / 2, 30, WHITE);
    }

    // Pause screen
    if (m_paused) {
        float buttonWidth = 200;
        float buttonHeight = 25;

        float x = (GetScreenWidth() - buttonWidth) / 2;
        float y = (GetScreenHeight() - buttonHeight) / 2;

        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, 150});

        if (GuiButton({x, y, buttonWidth, buttonHeight}, "Return to the game")) {
            m_paused ^= 1;
        }

        y += buttonHeight * 2 - 10;

        if (GuiButton({x, y, buttonWidth, buttonHeight}, "Exit")) {
            auto& mp = Multiplayer::get();

            mp.disconnect();
            m_scene = std::make_shared<MenuScene>();
            m_paused = false;
            m_level.clearLevel();
        }
    } else {
        if ((IsKeyDown(KEY_TAB) || m_end)) {
            drawScore();
        } 
    }
}

// TODO: Separete code and game scene
// --------------------
//       Editor
// --------------------
void Game::updateEditor() {
    auto width = GetScreenWidth();
    auto height = GetScreenHeight();

    m_camera.offset = Vector2 { width / 2.0f, height / 2.0f };
    
    if (m_testmode) {
        m_camera.target = Vector2 {m_player.x, m_player.y};

        Vector2 max = GetWorldToScreen2D(Vector2 { WORLD_SIZE, WORLD_SIZE }, m_camera);
        Vector2 min = GetWorldToScreen2D(Vector2 { 0, 0 }, m_camera);

        if (max.x < width) m_camera.offset.x = width - (max.x - (float)width/2);
        if (max.y < height) m_camera.offset.y = height - (max.y - (float)height/2);
        if (min.x > 0) m_camera.offset.x = (float)width/2 - min.x;
        if (min.y > 0) m_camera.offset.y = (float)height/2 - min.y;

        if (IsKeyPressed(KEY_Q)) {
            m_testmode = false;
        }

        m_player.onLadder = m_level.GetBlock(std::ceil(m_player.x), std::ceil(m_player.y)) == LADDER ||
            m_level.GetBlock(std::floor(m_player.x), std::floor(m_player.y)) == LADDER;

        if ((IsKeyDown(KEY_W) || IsKeyDown(KEY_SPACE) ||  IsKeyDown(KEY_UP))) {
            if (m_player.onLadder) {
                // TODO: Rewrite
                int playerX = m_level.GetBlock(std::ceil(m_player.x), std::ceil(m_player.y)) == LADDER ? std::ceil(m_player.x) :
                m_level.GetBlock(std::floor(m_player.x), std::floor(m_player.y)) == LADDER ? std::floor(m_player.x) : 0;

                m_player.speed.y = -0.2f;
                m_player.x = playerX;
            } else if (m_player.onGround) {
                m_player.speed.y = -0.3f;
            }
        }

        if (m_player.onLadder && (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))) {
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

        updatePlayer();
    } else {
        m_camera.target = Vector2 {m_cameraPos.x, m_cameraPos.y};
        Rectangle bg = { 0 };   
        bg.width = 200.f;
        bg.height = GetScreenHeight();
        bg.x = GetScreenWidth() - bg.width;
        bg.y = 0;

        Vector2 max = GetWorldToScreen2D(Vector2 { WORLD_SIZE + (bg.width / m_camera.zoom + 2), WORLD_SIZE }, m_camera);
        Vector2 min = GetWorldToScreen2D(Vector2 { 0.f, 0.f }, m_camera);

        if (max.x < width)  m_camera.offset.x = width - (max.x - width / 2.0f);
        if (max.y < height) m_camera.offset.y = height - (max.y - height / 2.0f);
        if (min.x > 0.0f)   m_camera.offset.x = width / 2.0f - min.x;
        if (min.y > 0.0f)   m_camera.offset.y = height / 2.0f - min.y;

        Vector2 mousePos = GetMousePosition();
        Vector2 worldMouse = GetScreenToWorld2D(mousePos, m_camera);
 
        static Vector2 prevMousePos = Vector2Zero();
        auto wheel = GetMouseWheelMove();

        Vector2 worldMousei = { 0 };
        worldMousei.x = (int) worldMouse.x;
        worldMousei.y = (int) worldMouse.y;

        if (m_coll) {
            for (int i = 0; i < COLLECTIBLIES_COUNT - 1; i++) {
                if (IsKeyPressed(KEY_ONE + i)) {
                    m_currentColl = (CollectibleType) (i + 1);
                    m_spawn = false;
                }
            }
        } else {
            for (int i = 0; i < BLOCKS_COUNT; i++) {
                if (IsKeyPressed(KEY_ONE + i)) {
                    m_currentBlock = (Block) i; 
                    m_spawn = false;
                }
            }
        }

        if (IsKeyDown(KEY_A)) {
            m_cameraPos.x -= 0.2f;
        }

        if (IsKeyDown(KEY_D)) {
            m_cameraPos.x += 0.2f;
        }

        if (IsKeyDown(KEY_W)) {
            m_cameraPos.y -= 0.2f;
        }

        if (IsKeyDown(KEY_S)) {
            m_cameraPos.y += 0.2f;
        }

        static bool prevRight  = false;
        static bool prevLeft   = false;

        static int ticksLeft   = 0;
        static int ticksRight  = 0;
        
        static int ticksBlocks = 0;

        if (!IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && prevRight && !CheckCollisionPointRec(mousePos, bg) && ticksBlocks == 0) {
            if (m_level.GetBlock(worldMousei.x, worldMousei.y)) {
                addElement(StackElement {.removed = true, .type = (Block)m_level.GetBlock(worldMousei.x, worldMousei.y), .position = worldMousei});
                m_level.setBlock(AIR, worldMousei.x, worldMousei.y);
            } else if (m_level.containsCollectible(worldMousei)) {
                addElement(StackElement {.removed = true, .type = m_level.getCollectible(worldMousei), .position = worldMousei});
                m_level.removeCollectible(worldMousei);
            } else if (m_level.GetBackgroundBlock(worldMousei.x, worldMousei.y)) {
                addElement(StackElement {.removed = true, .type = (Block)m_level.GetBackgroundBlock(worldMousei.x, worldMousei.y), .position = worldMousei, .background = true});
                m_level.setBackgroundBlock(AIR, worldMousei.x, worldMousei.y);
            } else if (m_level.containsSpawnpoint(worldMousei)) {
                addElement(StackElement {.removed = true, .type = Block::BLOCKS_COUNT, .position = worldMousei});
                m_level.removeSpawnpoint(worldMousei);
            }
        }

        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) && prevLeft && !CheckCollisionPointRec(mousePos, bg) && ticksBlocks == 0) {
            if (m_spawn) {
                m_level.setSpawnpoint({worldMousei.x, worldMousei.y});
                addElement(StackElement {.removed = false, .type = Block::BLOCKS_COUNT, .position = worldMousei});
            } else if (m_coll) {
                m_level.addCollectible({{worldMousei.x, worldMousei.y}, (CollectibleType)(m_currentColl + 1)});
                addElement(StackElement {.removed = false, .type = (CollectibleType)(m_currentColl + 1), .position = worldMousei});
            } else if (IsKeyDown(KEY_Q)) {
                m_level.setBackgroundBlock(m_currentBlock, worldMousei.x, worldMousei.y);
                addElement(StackElement {.removed = false, .type = m_currentBlock, .position = worldMousei, .background = true});
            } else {
                m_level.setBlock(m_currentBlock, worldMousei.x, worldMousei.y);
                addElement(StackElement {.removed = false, .type = m_currentBlock, .position = worldMousei, .background = false});
            }
        }

        if (IsKeyDown(KEY_LEFT_SHIFT) && !m_blocks) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                if (IsKeyDown(KEY_Q)) {
                    m_level.setBackgroundBlock(m_currentBlock, worldMousei.x, worldMousei.y);
                    addElement(StackElement {.removed = false, .type = m_currentBlock, .position = worldMousei, .background = true});
                } else {
                    m_level.setBlock(m_currentBlock, worldMousei.x, worldMousei.y);
                    addElement(StackElement {.removed = false, .type = m_currentBlock, .position = worldMousei, .background = false});
                }
            }
            
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                addElement(StackElement {.removed = true, .type = (Block)m_level.GetBlock(worldMousei.x, worldMousei.y), .position = worldMousei});
                m_level.setBlock(Block::AIR, worldMousei.x, worldMousei.y);
            }
        }
        
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !IsKeyDown(KEY_LEFT_SHIFT)) {
            if (!Vector2Equals(prevMousePos, mousePos)) {
                Vector2 delta = GetMouseDelta();
                delta = Vector2Scale(delta, -1.0f/m_camera.zoom);
                m_cameraPos = Vector2Add(m_cameraPos, delta);
            }
        }

        if (wheel != 0.f) {
            auto newZoom = m_camera.zoom + (wheel > 0.f ? 1 : -1) * 2.5f;

            m_camera.zoom = std::clamp(newZoom, 10.f, 100.f);
        }

        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_Z) && m_stack.size() > 0) {
            auto topElement = m_stack.front();
            auto type = topElement.type;
            
            if (std::holds_alternative<CollectibleType>(type)) {
                if (topElement.removed) {
                    m_level.addCollectible(Collectible {
                        .pos = topElement.position,
                        .type = std::get<CollectibleType>(topElement.type)
                    });
                } else {
                    m_level.removeCollectible(topElement.position);
                }
            } else if (std::holds_alternative<Block>(type)) {
                auto block = std::get<Block>(topElement.type);

                if (topElement.removed) {
                    if (block == BLOCKS_COUNT) {
                        m_level.setSpawnpoint(topElement.position);
                    } else {
                        if (topElement.background) {
                            m_level.setBackgroundBlock(block, topElement.position.x, topElement.position.y);
                        } else {
                            m_level.setBlock(block, topElement.position.x, topElement.position.y);
                        }
                    }
                } else {
                    if (block == BLOCKS_COUNT) {
                        m_level.removeSpawnpoint(topElement.position);
                    } else {
                        if (topElement.background) {
                            m_level.setBackgroundBlock(Block::AIR, topElement.position.x, topElement.position.y);
                        } else {
                            m_level.setBlock(Block::AIR, topElement.position.x, topElement.position.y);
                        }
                    }
                }
            }

            m_stack.pop_front();
        }

        m_cameraPos.x = std::clamp(m_cameraPos.x, (float) bg.width / m_camera.zoom + 2, (float) (WORLD_SIZE - bg.width / m_camera.zoom - 2));
        m_cameraPos.y = std::clamp(m_cameraPos.y, 0.f, (float) WORLD_SIZE);

        prevMousePos = mousePos;

        if (ticksRight <= 0) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                ticksRight = 7;
                prevRight = true;
            }
        } else {
            ticksRight--;

            if (ticksRight <= 0|| !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                prevRight = false;
                ticksRight = 0;
            }
        }

        if (ticksLeft <= 0) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                ticksLeft = 7;
                prevLeft = true;
            }
        } else {
            ticksLeft--;

            if (ticksLeft <= 0 || !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                prevLeft = false;
                ticksLeft = 0;
            }
        }

        if (m_blocks) {
            ticksBlocks = 6;
        } else if (ticksBlocks > 0) {
            ticksBlocks--;
        }

        printf("%d \n", ticksBlocks);
    }
}

void Game::renderEditor() {
    auto mousePos = GetMousePosition();

    Vector2 worldMouse = GetScreenToWorld2D(mousePos, m_camera);
    Vector2 worldMousei = { 0 };
    worldMousei.x = (int) worldMouse.x;
    worldMousei.y = (int) worldMouse.y;
    auto& rm = ResourceManager::get();

    ClearBackground({4, 4, 50, 255});

    Vector2 gunPos = {m_player.x + 0.5f, m_player.y + 0.5f};

    Vector2 direction = Vector2Subtract(worldMouse, gunPos);
    
    float angle = atan2f(direction.y, direction.x) * RAD2DEG;

    int flip = !(angle >= -90 && angle < 90) ? 1 : 0;

    BeginMode2D(m_camera);
        m_level.render();
    
        if (m_testmode) {
            rm.drawSpriteFromSheet(PLAYER_SPRITE, {m_player.x, m_player.y, 1.f, 1.f}, {0, 0}, 0, WHITE, flip);

            if (m_player.hat != NONE_HAT) {
                auto& hatPos = rm.getHatPos(m_player.hat);
                auto& hatSize = rm.getSpriteSize(rm.getHatSprite(m_player.hat));

                rm.drawSpriteFromSheet(rm.getHatSprite(m_player.hat), {m_player.x + hatPos.x / 8, m_player.y + hatPos.y / 8, hatSize.x / 8.f, hatSize.y / 8.f},
                {0, 0}, 0, WHITE, flip);
            }
        } else {
            // DrawRectangleLinesEx({(float)worldMousei.x, (float)worldMousei.y, 1.f, 1.f}, 0.1f, MAROON);
        }
    EndMode2D();    

    // GUI
    // TODO: Rewrite
    if (!m_testmode) {
        const float fontSize = 20.f;

        Rectangle bg = { 0 };
        bg.width = 300.f;
        bg.height = GetScreenHeight();
        bg.x = GetScreenWidth() - bg.width;
        bg.y = 0;

        DrawRectangleRec({bg}, {10, 10, 10, 255});

        DrawFPS(0, 0);
        DrawText(TextFormat("%f\n%f", m_cameraPos.x, m_cameraPos.y), 0, 20, 20, WHITE);
        DrawText(TextFormat("%f\n%f", worldMouse.x, worldMouse.y), 0, 60, 20, WHITE);

        float padding = 35.f;

        const char* text = m_coll ? "Current collectibles:" : "Current Block:";
        Vector2 blockTextSize = MeasureTextEx(GetFontDefault(), text, fontSize, fontSize / 10);
        
        float textX = bg.x + (bg.width - blockTextSize.x) / 2;
        float textY = padding;

        DrawText(text, textX, textY, fontSize, WHITE);
        
        if (m_coll && m_currentColl > 0) {
            Sprite sprite = Sprite::SHOTGUN_SPRITE;
            switch (m_currentColl) {
                case SHOTGUN_COLLECT: {
                    sprite = SHOTGUN_SPRITE;
                    break;
                }
                case SNIPER_RIFLE: {
                    sprite = SNIPER_RIFLE_SPRITE;
                    break;
                }

                case GRENADE_COLLECT: {
                    sprite = GRENADE_SPRITE;
                    break;
                }

                case MEDKIT: {
                    sprite = MEDKIT_SPRITE;
                    break;
                }
            }

            auto src = rm.getSpriteSize(sprite);
            Vector2 size = {src.x * 4, src.y * 4};

            Vector2 pos = {bg.x + (bg.width - size.x) / 2, padding + blockTextSize.y * 2};

            rm.drawSpriteFromSheet(sprite, {pos.x, pos.y, size.x, size.y});
        } else {
            if (m_currentBlock > 0) {
                Rectangle block = {
                    bg.x + (bg.width - 48.f) / 2,
                    textY + blockTextSize.y + padding / 2,
                    48.f,
                    48.f
                };

                if (m_currentBlock == BLOCKS_COUNT) {
                    DrawRectangleLinesEx(block, 5.f, MAROON);
                } else {
                    rm.drawBlock((Block)((int) (m_currentBlock) - 1), block);
                }
            }
        }
        

        constexpr std::size_t buttonsCount = 4;
        const Vector2 buttonSize = {bg.width - padding, padding};
        const std::array<const char*, buttonsCount> buttonsName = {
            "Play",
            "Save",
            "Collectibles",
            "Blocks"
        };

        for (int i = 0; i < buttonsCount; i++) {
            Rectangle buttonRec = {
                bg.x + (bg.width - buttonSize.x) / 2,
                padding * 4 + i * buttonSize.y + (padding / 2) * i,
                buttonSize.x,
                buttonSize.y
            };

            if (GuiButton(buttonRec, buttonsName.at(i))) {
                switch (i) {
                    // PLAY
                    case 0: {
                        if (m_level.getSpawnpoints().size() > 0) {
                            m_testmode = true;

                            m_player.x = m_level.getSpawnpoints().at(0).x;
                            m_player.y = m_level.getSpawnpoints().at(0).y;
                        }    
                        break;
                    }
                    // SAVE
                    case 1: {
                        m_level.write();
                        break;
                    }
                    // COLLECTIBLES
                    case 2: {
                        m_coll ^= 1;
                        m_spawn = false;
                        break;
                    }
                    // BLOCKS
                    case 3: {
                        m_blocks ^= 1;
                        break;
                    }

                    default: break;
                }
            }    
        }

        if (m_blocks) {
            const int bgX = 75;
            const int bgY = 75;
            const int bgWidth = GetScreenWidth() - 75 * 2;
            const int bgHeight = GetScreenHeight() - 75 * 2;

            DrawRectangle(bgX, bgY, bgWidth, bgHeight, {0, 0, 0, 100});

            for (int i = 0; i < BLOCKS_COUNT; i++) {
                float size = 48.f;
                float x = (int)(bgX + padding + (i % bgWidth) * size + padding * (i % bgWidth));
                float y = (int)(bgY + padding + i * size / bgHeight);
                Rectangle block = {x, y, size, size};

                if (i == BLOCKS_COUNT - 1) {
                    DrawRectangleLinesEx(block, 5.f, MAROON);
                    
                    if (CheckCollisionPointRec(mousePos, block)) {
                        DrawRectangleRec(block, {255, 255, 255, 50});

                        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                            m_currentBlock = BLOCKS_COUNT;
                            m_spawn = true;
                            m_coll = false;
                            m_blocks = false;
                        }
                    }                    
                } else {
                    rm.drawBlock((Block)i, {x, y, size, size});

                    if (CheckCollisionPointRec(mousePos, block)) {
                        DrawRectangleRec(block, {255, 255, 255, 50});

                        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                            m_currentBlock = (Block) (i + 1);
                            m_spawn = false;
                            m_coll = false;
                            m_blocks = false;
                        }
                    }
                }                
            }
        }
    } else {
        DrawText("Press Q to return in editor", 0, 0, 20, WHITE);
    }
}