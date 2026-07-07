#include "Types.hpp"
#include <Multiplayer.hpp>
#include <cstdio>
#include <Utils.hpp>
#include <Game.hpp>
#include <iostream>
#include <enet.h>
#include <iostream>
#include <zlib.h>

void Multiplayer::init(std::string nickname, std::string ip, int port) {
    if (enet_initialize() != 0) {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        exit(EXIT_FAILURE);
    }
    
    m_client = enet_host_create (NULL /* create a client host */,
            1 /* only allow 1 outgoing connection */,
            2 /* allow up 2 channels to be used, 0 and 1 */,
            0 /* assume any amount of incoming bandwidth */,
            0 /* assume any amount of outgoing bandwidth */);

    if (m_client == NULL) {
        exit(EXIT_FAILURE);
    }

    ENetAddress address;

    if (enet_address_set_host(&address, ip.c_str()) != 0) {
        fprintf(stderr, "Failed to resolve host %s\n", ip.c_str());

        return;
    }    
    address.port = port;

    m_peer = enet_host_connect(m_client, &address, 2, 0);

    if (m_peer == NULL) {
        printf("peer is null");
        exit(EXIT_FAILURE);
    }

    ENetEvent event;
    if (enet_host_service(m_client, &event, 10000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        printf("Connection to %s:%d succeeded \n", ip.c_str(), port);
    } else {
        printf("event.type=%d\n", event.type);
        enet_peer_reset(m_peer);
        printf("Connection to %s:%d failed \n", ip.c_str(), port);

        return;
    }

    std::cout << "Connected" << std::endl;

    m_connected = true;

    auto loginPacket = new char[HEADER_SIZE + nickname.length() + 1];
    memset(loginPacket, 0, HEADER_SIZE + nickname.length() + 1);

    loginPacket[0] = Header::AUTH;

    memcpy(loginPacket + 1, nickname.data(), nickname.length());

    sendPacket(loginPacket, HEADER_SIZE + nickname.length() + 1, ENET_PACKET_FLAG_RELIABLE);

    update();
}

void Multiplayer::update() {
    bool disconnected = false;
    
    ENetEvent event;

    while (true) {
        while(enet_host_service (m_client, &event, 0) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE: {
                    handlePacket(event.packet);
                    enet_packet_destroy(event.packet);

                    break;
                }

                case ENET_EVENT_TYPE_DISCONNECT: {
                    printf("Disconnection succeeded. \n");
                    disconnected = true;

                    break;
                }
                case ENET_EVENT_TYPE_NONE:
                    break;
                case ENET_EVENT_TYPE_CONNECT:
                    break;
                case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                    break;
            }
        }

        sleepMs(5);
    }

    if (!disconnected) {
        enet_peer_reset(m_peer);
    }
}

void Multiplayer::handlePacket(ENetPacket* packet) {     
    auto bytes = packet->data;

    auto header = bytes[0];

    // printf("Handling new packet with header %d... \n", header);

    bytes++;

    auto& game = Game::get();
    switch(header) {    
        case AUTH: {
            printf("Collecting players \n");

            auto num = *bytes;
            bytes++;
            
            for (int i = 0; i < num; i++) {
                auto id = *(uint32_t*)bytes;
                bytes += 4;

                std::cout << id << std::endl;

                auto x = *(float*)bytes;
                bytes += 4;
                auto y = *(float*)bytes;
                bytes += 4;

                auto nameLen = *(uint16_t*)bytes;
                bytes += 2;

                auto name = new char[nameLen + 1];
                name[nameLen] = 0;
                memcpy(name, bytes, nameLen);
                
                bytes += nameLen;

                auto hp = *(int*)bytes;
                bytes += 4;

                auto weapon = *(uint8_t*)bytes;
                bytes++;

                auto p = Player(name, x, y, hp, weapon);
                
                game.addPlayer(id, p);
            }

            break;
        }

        case ADDPLAYER: {
            auto x = *(float*)bytes;
            bytes += 4;
            
            auto y = *(float*)bytes;
            bytes += 4;

            auto id = *(uint32_t*)bytes;
            bytes += 4;
            
            auto nameLen = packet->dataLength - 13;

            printf("Data length: %lu \n", packet->dataLength);
            
            auto name = new char[nameLen];
            name[nameLen - 1] = 0;
            memcpy(name, bytes, nameLen);

            printf("My name: %s; Server name: %s \n", game.getPlayer().nickname.c_str(), name);
            if (name == game.getPlayer().nickname) {
                printf("its me \n");

                game.setMyId(id);
                game.getPlayer().x = x;
                game.getPlayer().y = y;
            } else {
                auto p = Player(name, x, y, 100);

                game.addPlayer(id, p);
            }

            break;
        }

        case MOVE: {
            auto id = *(uint32_t*)bytes;
            bytes += 4;
            auto x = *(float*)bytes;
            bytes += 4;
            auto y = *(float*)bytes;
            bytes += 4;

            
            if (id != game.getMyId()) {                
                game.updatePlayerPos(id, x, y);
            } else {
                game.getPlayer().x = x;
                game.getPlayer().y = y;
            }
            
            break;
        }

        case REMOVEPLAYER: {
            auto id = *(uint32_t*)bytes;
            bytes += 4;

            game.removePlayer(id);
            
            break;
        }

        case ADDBULLET: {
            auto index = *(uint32_t*)bytes;
            bytes += 4;

            auto x = *(float*)bytes;
            bytes += 4;
            auto y = *(float*)bytes;
            bytes += 4;

            auto velX = *(float*)bytes;
            bytes += 4;
            auto velY = *(float*)bytes;
            bytes += 4;
            auto angle = *(float*)bytes;
            bytes += 4;

            Vector2 bulletSize = {0.3f, 0.2f};

            game.getLevel().addBullet({{x, y}, {velX, velY}, index, angle * RAD2DEG});

            break;
        }

        case REMOVEBULLET: {
            auto index = *(uint32_t*)bytes;
            bytes += 4;

            game.getLevel().removeBullet(index);
            
            break;  
        }

        case UPDATEWEAPON: {            
            auto weaponID = *(uint8_t*)bytes;
            bytes++;
            
            auto id = *(uint32_t*)bytes;
            bytes += 4;

            if (id == game.getMyId()) {
                game.setMyWeapon(weaponID);
            } else {
                game.getPlayers().at(id).currentWeapon = weaponID;
            }

            break;
        }

        case ADDWEAPON: {
            auto weaponID = *(uint8_t*)bytes;
            bytes++;

            game.addWeapon(weaponID);
            
            break;
        }

        case LEVEL:  {
            auto size = *(uint32_t*)bytes;
            bytes += 4;
            std::vector<Bytef> decompressedData(WORLD_SIZE * WORLD_SIZE);

            std::vector<Bytef> compressedData;
            compressedData.insert(compressedData.begin(), bytes, bytes + size);

            uLongf uncomp =  WORLD_SIZE * WORLD_SIZE;

            int res = uncompress(decompressedData.data(), &uncomp, compressedData.data(), size);
            game.getLevel().setWorld(decompressedData);

            bytes += size;

            auto collectiblesSize = *(uint32_t*)bytes;
            bytes += 4;
            
            for (int i = 0; i < collectiblesSize; i++) {
                auto x = *(float*)bytes;
                bytes += 4;
                auto y = *(float*)bytes;
                bytes += 4;

                auto type = *(uint8_t*)bytes;
                bytes++;
                game.getLevel().addCollectible({{x, y}, (Collectibles) type, 0, y});
            }

            game.setLoaded(true);
            break;
        }

        case UPDATECOLLECTIBLE: {
            // NOTE: Maybe we can send just index?
            auto x = *(float*)bytes;
            bytes += 4;
            auto y = *(float*)bytes;
            bytes += 4;

            auto type = *(uint8_t*)bytes;
            bytes++;

            printf("Coll: %d \n", type);
            
            game.getLevel().editCollectible({x, y},(Collectibles)type);
            
            break;
        };

        case SETSCORE: {
            auto id = *(uint32_t*)bytes;
            bytes += 4;
            auto score = *(int*)bytes;
            bytes += 4;

            if (id != game.getMyId()) {
                game.setScore(id, score);
            } else {
                game.getPlayer().score = score;
            }
            
            break;
        }

        case SETHP: {
            auto id = *(uint32_t*)bytes;
            bytes += 4;
            auto hp = *(int*)bytes;
            bytes += 4;

            if (id != game.getMyId()) {
                game.setHp(id, hp);
            } else {
                game.getPlayer().hp = hp;
            }

            break;
        }

        case UPDATEANGLE: {
            auto id = *(uint32_t*)bytes;
            bytes += 4;
            auto hp = *(float*)bytes;
            bytes += 4;

            if (id != game.getMyId()) {
                game.setAngle(id, hp);
            }

            break;
        }
        default: break;
    }
}

void Multiplayer::sendPacket(char* data, int size, bool reliable) {
    if (m_connected) {
        ENetPacket* packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);

        enet_peer_send(m_peer, 0, packet);
    }
}