#pragma once
#include "Timer.hpp"
#include "Types.hpp"
#include <cstdint>
#include <raylib.h>
#include <unordered_map>
#include <Level.hpp>

class Game {
public:
    Game() : m_timer(TICKS) {}

    static inline Game& get() {
        static Game inst;
        return inst;
    }

    // TODO: Move the whole funcs to class Level
    void addPlayer(uint32_t id, Player player) { m_players.emplace(id, player); }
    void updatePlayerPos(uint32_t id, float x, float y) { m_players.at(id).x = x; m_players.at(id).y = y; }
    void removePlayer(uint32_t id) { m_players.erase(id); }
    
    void setMyId(uint32_t id) { m_myId = id; }
    uint32_t getMyId() { return m_myId; }
    Player& getPlayer() { return m_player; }
    void setMyWeapon(uint8_t weaponID) { 
        m_player.currentWeapon = weaponID;
    }
    void addWeapon(uint8_t weaponID) { m_player.inventory.at(weaponID) = true; }
    
    std::unordered_map<uint32_t, Player>& getPlayers() { return m_players; }
    Level& getLevel() { return m_level; }
    
    void setLoaded(bool loaded) { m_loaded = loaded; }
    void init(std::string nickname);

    void sendMovePacket();
    
    void setScore(uint32_t id, uint32_t score) { m_players.at(id).score = score; }
    void setHp(uint32_t id, int hp) { m_players.at(id).hp = hp; }
    void setAngle(uint32_t id, float angle) { m_players.at(id).angle = angle; }

    void renderEditor();
    void updateEditor();

    bool& getEditor() { return m_editor; }
    
    Camera2D& getCamera() { return m_camera; }

    void addMessage(std::string msg) { m_messages.push_back({msg, 5000.f}); }
private:
    Timer m_timer;
    
    Camera2D m_camera;
    // TODO: ifdef editor?
    RVector2 m_cameraPos;
    Block m_currentBlock;

    Level m_level;
    bool m_loaded;
    bool m_editor;
    bool m_spawn;

    Collectibles m_currentColl;
    bool m_coll;

    uint32_t m_myId;

    Player m_player;
    std::unordered_map<uint32_t, Player> m_players;

    char m_message[255];
    std::vector<Message> m_messages;
    bool m_chatOpened;

    void update();
    void render();
};