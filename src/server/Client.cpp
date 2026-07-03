#include "Client.hpp"
#include <Types.hpp>
#include <cmath>
#include <iostream>
#include "Server.hpp"
#include <Weapons.hpp>

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

        auto nicknameLen = packet->dataLength - 1;

        if (nicknameLen > 30) {
            // TODO: Send error to client
            return;
        }

        auto nickname = new char[nicknameLen];

        memcpy(nickname, bytes, nicknameLen);
        
        printf("Nickname: %s \n", nickname);    

        float x = 1;
        float y = 62;

        auto newPlayerSize = HEADER_SIZE + nicknameLen + sizeof(float) * 2 + sizeof(uint32_t);
        auto newPlayerPacket = new char[newPlayerSize];

        newPlayerPacket[0] = ADDPLAYER;

        auto newPlrIndex = 1;

        // x and y
        *(float*)(newPlayerPacket + newPlrIndex) = x;
        newPlrIndex += 4;
        *(float*)(newPlayerPacket + newPlrIndex) = y;
        newPlrIndex += 4;
        
        // peerID
        *(uint32_t*)(newPlayerPacket + newPlrIndex) = m_peer->connectID;
        newPlrIndex += 4;

        // nickname
        memcpy(newPlayerPacket + (newPlayerSize - nicknameLen), bytes, nicknameLen);

        srv.broadcast(newPlayerPacket, newPlayerSize);

        auto clientsCount = srv.getClients().size();

        auto playerPacketSize = HEADER_SIZE + sizeof(clientsCount) + (sizeof(Player) + sizeof(uint32_t)) * (clientsCount - 1);
        auto playersPacket = new char[playerPacketSize];

        playersPacket[0] = Header::AUTH;
        playersPacket[1] = (uint8_t)(srv.getClients().size() - 1);
        auto packetIndex = 2;

        m_player = {nickname, x, y, 100, 0, {GUN}};

        for (auto& [id, client] : srv.getClients()) {
            std::cout << id << std::endl;
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

        // auto weaponShotgun = new char[HEADER_SIZE + 1];
        // weaponShotgun[0] = ADDWEAPON;
        // weaponShotgun[1] = SNIPER_RIFLE;

        // m_player.inventory.push_back(SNIPER_RIFLE);

        // sendPacketTo(weaponShotgun, 2);

        m_loggedIn = true;
        delete[] playersPacket;
        delete[] newPlayerPacket;
        // delete[] weaponShotgun;
    } else { 
        switch (header) {
            case MOVE: {
                auto x = *(float*)bytes;
                bytes += 4;

                auto y = *(float*)bytes;
                bytes += 4;

                m_player.x = x;
                m_player.y = y;
                
                auto moveSize = HEADER_SIZE + sizeof(float) * 2 + sizeof(m_peer->connectID);
                auto movePlrPacket = new char[moveSize];
                
                movePlrPacket[0] = MOVE;
                
                *(uint32_t*)(movePlrPacket + 1) = m_peer->connectID;
                *(float*)(movePlrPacket + 5) = x;
                *(float*)(movePlrPacket + 9) = y;

                srv.broadcast(movePlrPacket, moveSize);

                delete [] movePlrPacket;

                break;
            }

            case ADDBULLET: {
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

                delete [] addBulletPacket;
                break;
            };
            case UPDATEWEAPON: {
                auto index = *(uint8_t*)bytes;

                if (index > m_player.inventory.size() || index > WEAPONS_COUNT) {
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