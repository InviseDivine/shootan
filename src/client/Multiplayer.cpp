#include "Types.hpp"
#include <Multiplayer.hpp>
#include <cstdio>
#include <Utils.hpp>
#include <Game.hpp>
#include <iostream>
#include <enet.h>
#include <iostream>
#include <zlib.h>
#include "ErrorScene.hpp"

std::string Multiplayer::getStringReason() {
    switch (m_reason) {
        case -2: {
            return "Timeout";
        }

        case WRONG_HEADER: {
            return "Wrong header";
        }

        case NICKNAME_TOO_LONG: {
            return "Nickname too long";
        }

        default: return "Unknown reason";
    }
}

void Multiplayer::init(std::string nickname, std::string ip, int port) {
    auto& game = Game::get();

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

    ENetAddress address = {};

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

    auto loginPacket = new char[HEADER_SIZE + nickname.length() + 2];
    memset(loginPacket, 0, HEADER_SIZE + nickname.length() + 2);

    loginPacket[0] = Header::AUTH;
    loginPacket[1] = game.getPlayer().hat;

    memcpy(loginPacket + 2, nickname.c_str(), nickname.length());

    sendPacket(loginPacket, HEADER_SIZE + nickname.length() + 2, ENET_PACKET_FLAG_RELIABLE);

    update();
}

void Multiplayer::update() {
    bool disconnected = false;
    
    ENetEvent event;

    while (!disconnected) {
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
                    m_reason = *(uint8_t*)(event.data);

                    break;
                }
                case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT: {
                    printf("Disconnection due to timeout. \n");
                    disconnected = true;

                    m_reason = -2;
                    break;
                }
                case ENET_EVENT_TYPE_NONE:
                    break;
                case ENET_EVENT_TYPE_CONNECT:
                    break;
            }
        }

        sleepMs(5);
    }

    if (!disconnected) {
        enet_peer_reset(m_peer);
    }

    auto& game = Game::get();

    game.pushScene(std::make_shared<ErrorScene>());
}

void Multiplayer::handlePacket(ENetPacket* packet) {     
    auto bytes = packet->data;

    auto header = bytes[0];

    // printf("Handling new packet with header %d... \n", header);

    bytes++;

    auto& game = Game::get();
    switch(header) {    
        // NOTE: Because ENet has big fucking scructure of packet, maybe better if we merge some of headers
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

                auto weapon = *(Weapons*)bytes;
                bytes++;

                auto hat = *(uint8_t*)bytes;
                bytes++;

                auto p = Player(name, x, y, hp, weapon);
                p.hat = (Hat) hat;
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
            
            auto hat = *(Hat*)bytes;
            bytes++;
            
            auto nameLen = packet->dataLength - 14;
            
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
                p.hat = hat;
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
            auto weaponID = *(Weapons*)bytes;
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
            auto weaponID = *(Weapons*)bytes;
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

            auto bgSize = *(uint32_t*)bytes;
            bytes += 4;

            std::vector<Bytef> decompressedBg(WORLD_SIZE * WORLD_SIZE);
            std::vector<Bytef> compressedBg;
            compressedBg.insert(compressedBg.begin(), bytes, bytes + bgSize);

            uLongf uncompBg = WORLD_SIZE * WORLD_SIZE;

            int resBg = uncompress(decompressedBg.data(), &uncompBg, compressedBg.data(), bgSize);
            game.getLevel().setBackground(decompressedBg);

            bytes += bgSize;

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
                game.addNotification(score);
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
                game.setMyHp(hp);
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

        case MESSAGE: {
            auto messageLength = packet->dataLength - 1;

            auto msg = new char[messageLength + 1];
            msg[messageLength] = 0;
            memcpy(msg, bytes, messageLength);

            game.addMessage(msg);

            break;
        }

        case DAMAGE: {
            auto id = *(uint32_t*)bytes;
            bytes += 4;
            auto damage = *(int*)bytes;
            bytes += 4;

            game.addNotification(id, damage);

            break;
        }

        case ROUNDEND: {                
            auto id = *(uint32_t*)bytes;
            bytes += 4; 

            if (id == 0) {
                game.setEnd(false, 0);
            } else {
                game.setEnd(true, id);
            }

            break;
        }

        case THROWGRENADE: {
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

            Vector2 bulletSize = {0.3f, 0.2f};
            Grenade grenade = {{velX, velY}, {x, y}};
            grenade.id = index;
            game.getLevel().addGrenade(grenade);
        
            break;
        }

        case ADDGRENADE: {
            auto id = *(Grenades*)bytes;
            game.getPlayer().grenade = id;

            break;
        }

        case REMOVEGRENADE: {
            auto id = *(uint32_t*)bytes;

            game.getLevel().removeGrenade(id);
            
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