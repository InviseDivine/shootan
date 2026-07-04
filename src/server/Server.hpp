#pragma once
#include <enet.h>
#include <unordered_map>
#include "Client.hpp"
#include "Level.hpp"
#include "Timer.hpp"

#define MAX_CLIENTS 32

class Server {
public:
    Server() : m_timer(TICKS) {}; 
    static inline Server& get() {
        static Server inst;
        return inst;
    }
    
    void broadcast(char* data, int size, bool reliable = true);

    int init();  
    std::unordered_map<uint32_t, Client>& getClients() { return m_clients; }
    Level& getLevel() { return m_level; }
private:
    ENetHost* m_server;
    Timer m_timer;
    
    Level m_level;
    std::unordered_map<uint32_t, Client> m_clients;

    int update();
};