#include "Types.hpp"
#include <Multiplayer.hpp>
#include <cstdio>
#include <Game.hpp>
#include <iostream>
#include <enet.h>
#include <iostream>
#include <zlib.h>
#include "ErrorScene.hpp"

void Multiplayer::disconnect() {
    if (m_peer) {
        enet_peer_disconnect(m_peer, 0);
    }
}

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
        std::cout << stderr << "An error occurred while initializing ENet.\n" << std::endl;
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
        std::cout << "Failed to resolve host " << ip << std::endl;

        return;
    }    

    address.port = port;

    m_peer = enet_host_connect(m_client, &address, 2, 0);

    if (m_peer == NULL) {
        std::cout << "Peer is null, exiting..." << std::endl;
        exit(EXIT_FAILURE);
    }

    ENetEvent event;
    if (enet_host_service(m_client, &event, 10000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        std::cout << "Connection to " <<  ip << ":" << port << " succeeded" << std::endl;
    } else {
        std::cout << "Event type = " << event.type << std::endl;
        enet_peer_reset(m_peer);
        std::cout << "Connection to " <<  ip << ":" << port << " failed" << std::endl;
        return;
    }

    std::cout << "Connected" << std::endl;

    m_connected = true;

    LoginPacket loginPacket {};
    std::copy(nickname.begin(), nickname.end(), loginPacket.username);
    loginPacket.hat = game.getPlayer().hat;

    sendPacket((char*)&loginPacket, sizeof(loginPacket), ENET_PACKET_FLAG_RELIABLE);

    update();
}

void Multiplayer::update() {
    bool disconnected = false;
    
    ENetEvent event;

    while (m_connected) {
        while(enet_host_service (m_client, &event, 20) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE: {
                    handlePacket(event.packet);
                    enet_packet_destroy(event.packet);

                    break;
                }

                case ENET_EVENT_TYPE_DISCONNECT: {
                    std::cout << "Disconnection succeeded." << std::endl;
                    disconnected = true;
                    m_connected = false;
                    break;
                }
                case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT: {
                    auto& game = Game::get();
                    std::cout << "Disconnection due to timeout." << std::endl;
                    disconnected = true;

                    m_reason = -2;
                    
                    game.pushScene(std::make_shared<ErrorScene>());

                    break;
                }
                case ENET_EVENT_TYPE_NONE:
                    break;
                case ENET_EVENT_TYPE_CONNECT:
                    break;
            }
        }
    }

    if (!disconnected) {
        enet_peer_reset(m_peer);
    }
}

void Multiplayer::handlePacket(ENetPacket* packet) {     
    auto bytes = packet->data;

    Header header = static_cast<Header>(packet->data[0]);

    // printf("Handling new packet with header %d... \n", header);

    auto& game = Game::get();

    // NOTE: Because ENet has big fucking scructure of packet, maybe better if we merge some headers
    switch(header) {    
        // TODO: Replace with struct or smth better :P
        case Header::ADDALLPLAYERS: {
            bytes++;

            auto num = *bytes;
            bytes++;
            
            for (int i = 0; i < num; i++) {
                // playerId
                auto id = *(uint32_t*)bytes;
                bytes += 4;

                // Position
                auto x = *(float*)bytes;
                bytes += 4;
                auto y = *(float*)bytes;
                bytes += 4;

                // Nickname
                auto nameLen = *(uint16_t*)bytes;
                bytes += 2;
                
                auto name = new char[nameLen + 1];
                name[nameLen] = 0;
                memcpy(name, bytes, nameLen);
                
                bytes += nameLen;

                // Hp
                auto hp = *(uint8_t*)bytes;
                bytes++;

                // Weapon
                auto weapon = *(WeaponType*)bytes;
                bytes++;

                // Hat
                auto hat = *(Hat*)bytes;
                bytes++;

                // Is dead
                auto isDead = *(uint8_t*)bytes;
                bytes++;

                auto p = Player {
                    .x = x,
                    .y = y,
                    .hp = hp,
                    .hat = hat,
                    .nickname = name,
                };

                p.isDied = isDead;
                 
                game.addPlayer(id, p);
            }

            break;
        }

        case Header::ADDPLAYER: {
            AddPlayerPacket* pack = reinterpret_cast<AddPlayerPacket*>(packet->data);
            
            if (pack->nickname == game.getPlayer().nickname) {
                game.setMyId(pack->peerId);

                game.getPlayer().x = pack->position.x;
                game.getPlayer().y = pack->position.y;
            } else {
                auto p = Player {
                    .x = pack->position.x,
                    .y = pack->position.y,
                    .hp = 100,
                    .hat = pack->hat,
                    .nickname = pack->nickname
                };

                game.addPlayer(pack->peerId, p);
            }

            break;
        }

        case Header::MOVE: {
            MovePacket* pack = reinterpret_cast<MovePacket*>(packet->data);
            
            if (pack->id != game.getMyId()) {    
                game.updatePlayerPos(pack->id, pack->position.x, pack->position.y);
            } else {
                game.getPlayer().x = pack->position.x;
                game.getPlayer().y = pack->position.y;
            }
            
            break;
        }

        case Header::REMOVEPLAYER: {
            RemovePlayerPacket* pack = reinterpret_cast<RemovePlayerPacket*>(packet->data);

            game.removePlayer(pack->id);
            
            break;
        }

        case Header::ADDBULLET: {
            AddBulletPacket* pack = reinterpret_cast<AddBulletPacket*>(packet->data);

            game.getLevel().addBullet({
                pack->position, 
                pack->velocity, 
                pack->id, 
                pack->angle * RAD2DEG
            });

            break;
        }

        case Header::REMOVEBULLET: {
            RemoveBulletPacket* pack = reinterpret_cast<RemoveBulletPacket*>(packet->data);
            game.getLevel().removeBullet(pack->bulletId);
            
            break;  
        }

        case Header::UPDATEWEAPON: {            
            UpdateWeaponPacket* pack = reinterpret_cast<UpdateWeaponPacket*>(packet->data);

            if (pack->id == game.getMyId()) {
                game.setMyWeapon(pack->index);
            } else {
                game.getPlayers().at(pack->id).currentWeapon = pack->index;
            }

            break;
        }

        case Header::ADDWEAPON: {
            AddWeaponPacket* pack = reinterpret_cast<AddWeaponPacket*>(packet->data);
            game.addWeapon(pack->type);
            
            break;
        }

        // TODO: Replace with struct or smth better :P
        case Header::LEVEL:  {
            bytes++;
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
                game.getLevel().addCollectible({{x, y}, (CollectibleType) type, 0, y});
            }

            game.setLoaded(true);
            break;
        }

        case Header::UPDATECOLLECTIBLE: {
            // NOTE: Maybe we can send just index?
            UpdateCollectiblePacket* pack = reinterpret_cast<UpdateCollectiblePacket*>(packet->data);
            game.getLevel().editCollectible(pack->position, pack->type);
            
            break;
        };

        case Header::SETSCORE: {
            ScorePacket* pack = reinterpret_cast<ScorePacket*>(packet->data);
            
            if (pack->deadId == game.getMyId()) {
                game.setMyDied(true);
            } else {
                game.setDied(pack->deadId, true);
            }
            
            if (pack->killerId != game.getMyId()) {
                game.setScore(pack->killerId, pack->score);
            } else {
                game.addNotification(pack->score);
            }
            
            break;
        }

        case Header::SETHP: {
            SetHpPacket* pack = reinterpret_cast<SetHpPacket*>(packet->data);
            if (pack->id != game.getMyId()) {
                game.setHp(pack->id, pack->hp);
            } else {
                game.setMyHp(pack->hp);
            }

            break;
        }

        case Header::UPDATEANGLE: {
            UpdateAnglePacket* pack = reinterpret_cast<UpdateAnglePacket*>(packet->data);

            if (pack->id != game.getMyId()) {
                game.setAngle(pack->id, pack->angle);
            }

            break;
        }

        case Header::MESSAGE: {
            MessagePacket* pack = reinterpret_cast<MessagePacket*>(packet->data);
            game.addMessage(pack->message);

            break;
        }

        case Header::DAMAGE: {
            DamagePacket* pack = reinterpret_cast<DamagePacket*>(packet->data);
            game.addNotification(pack->damageReceiver, pack->damage);

            break;
        }

        case Header::ROUNDEND: {                
            RoundEndPacket* pack = reinterpret_cast<RoundEndPacket*>(packet->data);

            if (pack->winner == 0) {
                game.setEnd(false, 0);
            } else {
                game.setEnd(true, pack->winner);
            }

            break;
        }

        case Header::ADDGRENADE: {
            AddGrenadePacket* pack = reinterpret_cast<AddGrenadePacket*>(packet->data);
            Grenade grenade = {
                .velocity = pack->velocity,
                .pos = pack->position,
                .id = pack->id,
            };

            game.getLevel().addGrenade(grenade);
        
            break;
        }

        case Header::ADDGRENADEINVENTORY: {
            AddGrenadeInventoryPacket* pack = reinterpret_cast<AddGrenadeInventoryPacket*>(packet->data);
            game.getPlayer().grenade = pack->type;

            break;
        }

        case Header::REMOVEGRENADE: {
            RemoveGrenadePacket* pack = reinterpret_cast<RemoveGrenadePacket*>(packet->data);
            game.getLevel().removeGrenade(pack->grenadeId);
            
            break;
        }

        case Header::REVIVE: {
            RevivePacket* pack = reinterpret_cast<RevivePacket*>(packet->data);

            if (pack->id != game.getMyId()) {      
                game.setHp(pack->id, 100);   
                game.setDied(pack->id, false);       
                game.updatePlayerPos(pack->id, pack->position.x, pack->position.y);
            } else {
                game.getPlayer().x = pack->position.x;
                game.getPlayer().y = pack->position.y;
                game.getPlayer().hp = 100;
                game.setMyDied(false);
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