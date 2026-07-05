#include "Client.hpp"
#include <Types.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include "Level.hpp"
#include "Server.hpp"
#include <Weapons.hpp>
#include <zlib.h>

void Client::packetReceived(ENetPacket* packet) {
    auto bytes = packet->data;
    auto header = *(Header*)bytes;

    // printf("New packet from %u with header %d \n", m_peer->connectID, header);

    auto& srv = Server::get();

    bytes++;

    if (!m_loggedIn) {
        if (header != AUTH) {
            return;
        }

        auto& clients = srv.getClients();

        auto nicknameLen = packet->dataLength - 1;

        if (nicknameLen > 30) {
            // TODO: Send error to client and disconnect it
            return;
        }

        auto nickname = new char[nicknameLen];

        memcpy(nickname, bytes, nicknameLen);

        auto contains = std::find_if(clients.begin(), clients.end(), [&nickname](const auto& pair) {
            return pair.second.m_player.nickname == nickname;
        });

        if (contains != clients.end()) {
            return; 
        }
        
        auto& collectibles = srv.getLevel().getCollectibles();

        uLongf ogLvlSize = WORLD_SIZE * WORLD_SIZE;
        uLongf compressed = compressBound(ogLvlSize);
        std::vector<Bytef> compressedData(compressed);

        int res = compress(compressedData.data(), &compressed, 
                        (const Bytef*)srv.getLevel().getWorld().data(), ogLvlSize);
        compressedData.resize(compressed);
        auto lvlSize = 
            HEADER_SIZE +
            sizeof(uint32_t) + 
            compressed + 
            sizeof(uint32_t) +
            collectibles.size() * (sizeof(float) * 2 + sizeof(Collectibles));

        auto levelPacket = new char[lvlSize];
        
        levelPacket[0] = Header::LEVEL; 

        *(uint32_t*)(levelPacket + 1) = compressed;
        char* dst = levelPacket + 5;

        memcpy(dst, compressedData.data(), compressed);
        
        auto i = HEADER_SIZE + sizeof(uint32_t) + compressed;

        *(uint32_t*)(levelPacket + i) = collectibles.size();
        i += 4;

        for (auto& coll : collectibles) {
            *(float*)(levelPacket + i) = coll.pos.x;
            i += 4;
            
            *(float*)(levelPacket + i) = coll.pos.y;
            i += 4;

            *(Collectibles*)(levelPacket + i) = coll.type;
            i++;
        }

        sendPacketTo(levelPacket, lvlSize);
        
        auto newPlayerSize = HEADER_SIZE + nicknameLen + sizeof(float) * 2 + sizeof(uint32_t);
        auto newPlayerPacket = new char[newPlayerSize];

        newPlayerPacket[0] = ADDPLAYER;

        auto newPlrIndex = 1;

        auto& pos = srv.getLevel().getRandomSpawn();

        // x and y
        *(float*)(newPlayerPacket + newPlrIndex) = pos.x;
        newPlrIndex += 4;
        *(float*)(newPlayerPacket + newPlrIndex) = pos.y;
        newPlrIndex += 4;
        
        // peerID
        *(uint32_t*)(newPlayerPacket + newPlrIndex) = m_peer->connectID;
        newPlrIndex += 4;

        // nickname
        memcpy(newPlayerPacket + (newPlayerSize - nicknameLen), bytes, nicknameLen);

        srv.broadcast(newPlayerPacket, newPlayerSize);

        auto clientsCount = clients.size();

        auto playerPacketSize = HEADER_SIZE + sizeof(clientsCount) + (sizeof(Player) + sizeof(uint32_t)) * (clientsCount - 1);
        auto playersPacket = new char[playerPacketSize];

        playersPacket[0] = Header::AUTH;
        playersPacket[1] = (uint8_t)(clients.size() - 1);
        auto packetIndex = 2;

        m_player = {nickname, pos.x, pos.y, 100, 0, {GUN}};

        for (auto& [id, client] : clients) {
            if (id != m_peer->connectID) {
                *(uint32_t*)(playersPacket + packetIndex) = id;
                packetIndex += 4;

                *(float*)(playersPacket + packetIndex) = client.m_player.x;
                packetIndex += 4;
                *(float*)(playersPacket + packetIndex) = client.m_player.y;
                packetIndex += 4;

                *(uint16_t*)(playersPacket + packetIndex) = client.m_player.nickname.length();
                packetIndex += 2;
                memcpy(playersPacket + packetIndex, client.m_player.nickname.c_str(), client.m_player.nickname.length());
                packetIndex += client.m_player.nickname.length();

                *(int*)(playersPacket + packetIndex) = client.m_player.hp;
                packetIndex += 4;
                *(uint8_t*)(playersPacket + packetIndex) = client.m_player.currentWeapon;
                packetIndex++;
            } 
        }

        sendPacketTo(playersPacket, playerPacketSize);

        m_loggedIn = true;
        delete[] playersPacket;
        delete[] newPlayerPacket;
        delete[] levelPacket;
    } else { 
        switch (header) {
            case MOVE: {
                bool shouldExclude = true;

                auto x = *(float*)bytes;
                bytes += 4;

                auto y = *(float*)bytes;
                bytes += 4;
                
                if (x <= 0 || x >= WORLD_SIZE) return;

                m_player.x = x;
                m_player.y = y;

                
                if (y > WORLD_SIZE) {
                    auto& lvl = srv.getLevel();
                    auto& pos = lvl.getRandomSpawn();

                    m_player.x = pos.x;
                    m_player.y = pos.y;

                    shouldExclude = false;
                }
                
                auto moveSize = HEADER_SIZE + sizeof(float) * 2 + sizeof(m_peer->connectID);
                auto movePlrPacket = new char[moveSize];
                
                movePlrPacket[0] = MOVE;
                
                *(uint32_t*)(movePlrPacket + 1) = m_peer->connectID;
                *(float*)(movePlrPacket + 5) = x;
                *(float*)(movePlrPacket + 9) = y;

                if (shouldExclude) {
                    srv.broadcastWithExclude(movePlrPacket, moveSize, m_peer->connectID);
                } else {
                    srv.broadcast(movePlrPacket, moveSize);
                }

                delete [] movePlrPacket;

                break;
            }

            case ADDBULLET: {
                if (m_player.reload <= 0) {
                    auto angle = *(float*)bytes;
                    auto& level = srv.getLevel();

                    RVector2 gunPos = {m_player.x + 0.5f, m_player.y + 0.5f};
                    Weapon& wpn = weapons.at(m_player.currentWeapon);

                    Bullet bullet = Bullet {
                        {gunPos.x, gunPos.y},   
                        {cosf(angle) * wpn.bulletSpeed, sinf(angle) * wpn.bulletSpeed}, 
                        static_cast<uint32_t>(level.bulletSize()),
                        wpn.lifeTime, 
                        m_peer->connectID,
                        m_player.currentWeapon
                    };
                                    
                    level.addBullet(bullet);

                    auto addBulletPacket = new char[HEADER_SIZE + 4 + 8 + 8];
                    addBulletPacket[0] = Header::ADDBULLET;
                    
                    uint32_t size = bullet.id;

                    *(uint32_t*)(addBulletPacket + HEADER_SIZE) = size;
                    *(float*)(addBulletPacket + HEADER_SIZE + 4) = bullet.pos.x;
                    *(float*)(addBulletPacket + HEADER_SIZE + 8) = bullet.pos.y;
                    *(float*)(addBulletPacket + HEADER_SIZE + 12) = bullet.velocity.x;
                    *(float*)(addBulletPacket + HEADER_SIZE + 16) = bullet.velocity.y;

                    auto& srv = Server::get();

                    srv.broadcast(addBulletPacket, HEADER_SIZE + 4 + 8 + 8);

                    m_player.reload = wpn.reloadTime;
                    
                    delete [] addBulletPacket;
                    break;
                }
            };
            case UPDATEWEAPON: {
                auto index = *(uint8_t*)bytes;

                if (index > m_player.inventory.size() || index > WEAPONS_COUNT || m_player.currentWeapon == index) {
                    return;
                }

                m_player.currentWeapon = index;
                auto updateWeapon = new char[HEADER_SIZE + sizeof(uint8_t) + sizeof(m_peer->connectID)];

                updateWeapon[0] = UPDATEWEAPON;
                updateWeapon[1] = m_player.inventory.at(index);
                *(uint32_t*)(updateWeapon + 2) = m_peer->connectID;

                srv.broadcast(updateWeapon, HEADER_SIZE + sizeof(uint8_t) + sizeof(m_peer->connectID));

                delete [] updateWeapon;

                break;
            }
            default:
                break;
        }
    }
}

int Client::sendPacketTo(char* data, int size, bool reliable) {
    auto enetPacket = enet_packet_create(data, size, reliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);

    return enet_peer_send(m_peer, 0, enetPacket) == 0;
}