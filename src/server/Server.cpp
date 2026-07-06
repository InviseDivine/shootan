#include "Server.hpp"
#include "Types.hpp"
#include <enet.h>
#include <iostream>
#include <Utils.hpp>
#include <random>

int Server::init() {
    // std::signal(SIGINT, signalHandler);

    if (enet_initialize() != 0) {
        std::cout << "An error occurred while initializing ENet.\n" << std::endl;
        return 1;
    }

    ENetAddress address = {0};

    address.host = ENET_HOST_ANY; /* Bind the server to the default localhost.     */
    address.port = 6890;          /* Bind the server to port 7777. */

    /* create a server */
    m_server = enet_host_create(&address, MAX_CLIENTS, 2, 0, 0);

    if (m_server == NULL) {
        std::cout << "An error occurred while trying to create an ENet server host. Maybe port already listening?" << std::endl;
        return 1;
    }

    printf("Starting server on port %d...\n", address.port);

    float y = WORLD_SIZE - 1;

    // floor main
    for (int i = 0; i < WORLD_SIZE; i++) {
        m_level.getWorld().at(PACK_INDEX(i, y, WORLD_SIZE)) = 1;
    }

    // floor
    for (int i = 59; i > 46; i--) {
        m_level.getWorld().at(PACK_INDEX(i, 123, WORLD_SIZE)) = BRICK;
    }
    
    // wall 1
    for (int i = 124; i > 117; i--) {
        m_level.getWorld().at(PACK_INDEX(59, i, WORLD_SIZE)) = BRICK;
    }
    
    // wall 2
    for (int i = 124; i > 117; i--) {
        m_level.getWorld().at(PACK_INDEX(47, i, WORLD_SIZE)) = BRICK;
    }

    // floor 2
    for (int i = 59; i > 46; i--) {
        m_level.getWorld().at(PACK_INDEX(i, 117, WORLD_SIZE)) = BRICK;
    }

    // wall 1
    for (int i = 117; i > 106; i--) {
        m_level.getWorld().at(PACK_INDEX(59, i, WORLD_SIZE)) = BRICK;
    }
    
    // wall 2
    for (int i = 117; i > 106; i--) {
        m_level.getWorld().at(PACK_INDEX(47, i, WORLD_SIZE)) = BRICK;
    }

    // floor 3
    for (int i = 59; i > 46; i--) {
        m_level.getWorld().at(PACK_INDEX(i, 112, WORLD_SIZE)) = BRICK;
    }

    // ceiling
    for (int i = 59; i > 46; i--) {
        m_level.getWorld().at(PACK_INDEX(i, 106, WORLD_SIZE)) = BRICK;
    }

    m_level.getWorld().at(PACK_INDEX(52, 123, WORLD_SIZE)) = AIR;
    m_level.getWorld().at(PACK_INDEX(54, 123, WORLD_SIZE)) = AIR;

    m_level.getWorld().at(PACK_INDEX(52, 117, WORLD_SIZE)) = AIR;
    m_level.getWorld().at(PACK_INDEX(54, 117, WORLD_SIZE)) = AIR;

    m_level.getWorld().at(PACK_INDEX(52, 112, WORLD_SIZE)) = AIR;
    m_level.getWorld().at(PACK_INDEX(54, 112, WORLD_SIZE)) = AIR;

    m_level.getWorld().at(PACK_INDEX(59, 109, WORLD_SIZE)) = AIR;
    m_level.getWorld().at(PACK_INDEX(59, 110, WORLD_SIZE)) = AIR;
    m_level.getWorld().at(PACK_INDEX(59, 111, WORLD_SIZE)) = AIR;

    m_level.getWorld().at(PACK_INDEX(62, 111, WORLD_SIZE)) = BRICK;
    m_level.getWorld().at(PACK_INDEX(63, 111, WORLD_SIZE)) = BRICK;

    m_level.getWorld().at(PACK_INDEX(66, 112, WORLD_SIZE)) = BRICK;
    m_level.getWorld().at(PACK_INDEX(67, 112, WORLD_SIZE)) = BRICK;
    m_level.getWorld().at(PACK_INDEX(68, 112, WORLD_SIZE)) = BRICK;


    m_level.getWorld().at(PACK_INDEX(72, 111, WORLD_SIZE)) = BRICK;
    m_level.getWorld().at(PACK_INDEX(73, 111, WORLD_SIZE)) = BRICK;
    m_level.getWorld().at(PACK_INDEX(74, 111, WORLD_SIZE)) = BRICK;

    for (int i = 126; i > 110; i--) {
        m_level.getWorld().at(PACK_INDEX(53, i, WORLD_SIZE)) = LADDER;
    }

    for (int x = 80; x < 91; x++) {
        m_level.getWorld().at(PACK_INDEX(x, 123, WORLD_SIZE)) = BRICK;
    }

    m_level.getWorld().at(PACK_INDEX(80, 124, WORLD_SIZE)) = BRICK;
    m_level.getWorld().at(PACK_INDEX(90, 124, WORLD_SIZE)) = BRICK;

    // ---------------------
    for (int x = 108; x < 117; x++) {
        m_level.getWorld().at(PACK_INDEX(x, y, WORLD_SIZE)) = BRICK;
    }
    
    for (int x = 108; x < 117; x++) {
        m_level.getWorld().at(PACK_INDEX(x, 121, WORLD_SIZE)) = BRICK;
    }
    
    for (int i = 124; i > 115; i--) {
        m_level.getWorld().at(PACK_INDEX(108, i, WORLD_SIZE)) = BRICK;
    }

    for (int i = 124; i > 115; i--) {
        m_level.getWorld().at(PACK_INDEX(116, i, WORLD_SIZE)) = BRICK;
    }

    for (int x = 108; x < 116; x++) {
        m_level.getWorld().at(PACK_INDEX(x, 117, WORLD_SIZE)) = BRICK;
    }

    for (int i = 126; i > 115; i--) {
        m_level.getWorld().at(PACK_INDEX(112, i, WORLD_SIZE)) = LADDER;
    }
    // m_level.getWorld().at(PACK_INDEX(108, 124, WORLD_SIZE)) = BRICK;
    // m_level.getWorld().at(PACK_INDEX(115, 124, WORLD_SIZE)) = BRICK;

    float x = WORLD_SIZE / 2 - 1;

    // Collectibles
    m_level.getCollectibles().push_back({{49, 116}, SHOTGUN_COLLECT});
    m_level.getCollectibles().push_back({{73, 110}, SNIPER_COLLECT});
    m_level.getCollectibles().push_back({{114, 126}, SHOTGUN_COLLECT});

    // Respawn points
    m_level.addPoint({x, y - 1.f});
    m_level.addPoint({85, 122});
    m_level.addPoint({110, 116});

    return update();
}

void Server::broadcast(char* data, int size, bool reliable) {
    uint32_t flag = (reliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
    auto enetPacket = enet_packet_create(data, size, flag);

    enet_host_broadcast(m_server, static_cast<uint8_t>(0), enetPacket);
}

// private
int Server::update() {
    while (true) {
        m_timer.advanceTime();
        
        for (uint32_t i = 0; i < m_timer.getTicks(); i++) {
            m_level.update();
        }
        
        ENetEvent event;

        while (enet_host_service(m_server, &event, 0) > 0) {
            auto id = event.peer->connectID;

            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                if (!m_clients.contains(id)) {
                    event.peer->data = (void*)((uintptr_t)id);

                    m_clients.emplace(id, Client(event.peer));
                }

                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT: {
                auto disconnectID = (uint32_t)((uintptr_t)event.peer->data);

                printf("disconnecting id: %u \n", disconnectID);
                
                if (m_clients.contains(disconnectID)) {
                    // sendServerMessage(std::format("{} disconnected from the server", m_clients.at(disconnectID).m_player.nickname));

                    std::erase_if(m_clients, [&](auto& pair) {
                        return pair.second.getID() == disconnectID;
                    });

                    auto removePlayerPacket = new char[HEADER_SIZE + 4];
                    removePlayerPacket[0] = Header::REMOVEPLAYER;

                    *(uint32_t*)(removePlayerPacket + HEADER_SIZE) = disconnectID;

                    broadcast(removePlayerPacket, HEADER_SIZE + 4);
                }

                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT: {
                auto disconnectID = (uint32_t)((uintptr_t)event.peer->data);

                printf("disconnecting timeout id: %u \n", disconnectID);

                if (m_clients.contains(disconnectID)) {
                    // sendServerMessage(std::format("{} disconnected from the server due timeout", m_clients.at(disconnectID).m_player.nickname));

                    std::erase_if(m_clients, [&](auto& pair) {
                        return pair.second.getID() == disconnectID;
                    });

                    auto removePlayerPacket = new char[HEADER_SIZE + 4];
                    removePlayerPacket[0] = Header::REMOVEPLAYER;

                    *(uint32_t*)(removePlayerPacket + HEADER_SIZE) = disconnectID;

                    broadcast(removePlayerPacket, HEADER_SIZE + 4);
                }

                break;
            }

            case ENET_EVENT_TYPE_RECEIVE: {
                if (m_clients.contains(id)) {
                    m_clients[id].packetReceived(event.packet);
                }
                
                enet_packet_destroy(event.packet);
            }

            default: break;
            }
        }

        sleepMs(20);
    }

    printf("Stopping server...");

    enet_host_destroy(m_server);
    enet_deinitialize();

    return 0;
}

void Server::broadcastWithExclude(char* data, int size, uint32_t excludePeer) {
    for (auto& [id, client] : m_clients) {
        if (id != excludePeer) {
            client.sendPacketTo(data, size);
        }
    }
}

void Server::disconnectPeer(ENetPeer* peer) {
    ENetEvent event;
    
    enet_peer_disconnect(peer, 0);

    while (enet_host_service(m_server, &event, 3000) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy (event.packet);
                break;
        
            case ENET_EVENT_TYPE_DISCONNECT:
                puts ("Disconnection succeeded.");
                return;
        }
    }
    enet_peer_reset (peer);
}