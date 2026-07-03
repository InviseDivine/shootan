#include "Server.hpp"
#include <iostream>
#include <Utils.hpp>

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
        printf("An error occurred while trying to create an ENet server host. Maybe port already listening?\n");

        return 1;
    }

    printf("Starting server on port %d...\n", address.port);

    for (int i = 0; i < 64; i++) {
        m_level.getWorld().at(PACK_INDEX(i, 63, 63)) = 1;
    }

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