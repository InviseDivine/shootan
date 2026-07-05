#pragma once

#include "Types.hpp"
#include <enet.h>

class Client {
public:
    Client(ENetPeer* peer) : m_peer(peer), m_loggedIn(false), m_player({""}) {}
    Client() : Client(nullptr) {}

    void packetReceived(ENetPacket* packet);
    int sendPacketTo(char* data, int size, bool reliable = true);

    uint32_t getID() { return m_peer->connectID; }

    Player m_player;
private:
    ENetPeer* m_peer;
    bool m_loggedIn;
};