#pragma once
#include "Types.hpp"
#include <cstdint>
#include <raylib.h>
#include <unordered_map>
#include <Level.hpp>
#include <iostream>

class Game {
public:
    static inline Game& get() {
        static Game inst;
        return inst;
    }

    void addPlayer(uint32_t id, Player player) { m_players.emplace(id, player); }
    void updatePlayerPos(uint32_t id, float x, float y) { m_players.at(id).x = x; m_players.at(id).y = y; }
    void removePlayer(uint32_t id) { m_players.erase(id); }
    
    void setMyId(uint32_t id) { m_myId = id; }
    uint32_t getMyId() { return m_myId; }
    Player& getPlayer() { return m_player; }
    void setMyWeapon(uint8_t weaponID) { 
        std::cout << "tedt" << std::endl;

        for (int i = 0; i < m_player.inventory.size(); i++) {
            if (m_player.inventory.at(i) == weaponID) {
                m_player.currentWeapon = i;
                break;
            }
        }
    }
    void addWeapon(uint8_t weaponID) { m_player.inventory.push_back(weaponID); 
    std::cout << m_player.inventory.size() << std::endl; }
    
    std::unordered_map<uint32_t, Player>& getPlayers() { return m_players; }
    Level& getLevel() { return m_level; }

    void init(std::string nickname);
    
    
    Texture2D& getTexture(Collectible id) { return m_textures.at(id); };
    // Texture2D& getWeaponTextureFromId(Weapon id);
    void sendMovePacket();
private:

    Camera2D m_camera;

    Level m_level;

    uint32_t m_myId;

    Player m_player;
    std::unordered_map<uint32_t, Player> m_players;

    std::array<Texture2D, COLLECTIBLIES_COUNT> m_textures;
        
    void update();
    void render();
};