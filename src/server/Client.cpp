#include "Client.hpp"
#include <Types.hpp>
#include <algorithm>
#include <cmath>
#include "Level.hpp"
#include "Server.hpp"
#include <Weapons.hpp>
#include <zlib.h>
#include <cmath>
#include <iostream>
   
#define PI 3.14159265358979323846f
#define RAD2DEG (180.0f/PI)
#define MAX_NAME_LEN 30

std::array<Vector2, WEAPONS_COUNT> weaponsSize {{
    {1.2f, 0.72f},  // PISTOL
    {1.44f, 0.48f},  // SHOTGUN
    {1.92f, 0.48f}, // SNIPER_RIFLE
}};

void sendBullet(Bullet& bullet, float angle) {
    AddBulletPacket packet {
        .angle = angle,
        .id = bullet.id,
        .position = bullet.pos,
        .velocity = bullet.velocity,
    };

    auto& srv = Server::get();

    srv.broadcast((char*)&packet, sizeof(packet));
}

void Client::packetReceived(ENetPacket* packet) {
    Header header = static_cast<Header>(packet->data[0]);

    // printf("New packet from %u with header %d \n", m_peer->connectID, header);

    auto& srv = Server::get();

    if (!m_loggedIn) {
        if (header != AUTH) {
            // enet_peer_disconnect(m_peer, 0);
            return;
        }

        LoginPacket* pack = reinterpret_cast<LoginPacket*>(packet->data);

        auto& clients = srv.getClients();
        
        auto hat = pack->hat;

        auto plrNick = pack->username;

        auto contains = std::find_if(clients.begin(), clients.end(), [plrNick](const auto& pair) {
            return pair.second.m_player.nickname == plrNick;
        });

        if (contains != clients.end()) {
            return; 
        }

        auto& collectibles = srv.getLevel().getCollectibles();

        uLongf ogLvlSize = WORLD_SIZE * WORLD_SIZE;
        
        uLongf compressedBgSize = compressBound(ogLvlSize);
        uLongf compressed = compressBound(ogLvlSize);
        
        std::vector<Bytef> compressedData(compressed);
        std::vector<Bytef> compressedBg(compressedBgSize);

        int res = compress(compressedData.data(), &compressed, 
                        (const Bytef*)srv.getLevel().getWorld().data(), ogLvlSize);

        int resBg = compress(compressedBg.data(), &compressedBgSize, 
                        (const Bytef*)srv.getLevel().getBackground().data(), ogLvlSize);
        
        compressedBg.resize(compressedBgSize);
        compressedData.resize(compressed);

        // TODO: Replace with struct or smth better :P
        auto lvlSize = 
            HEADER_SIZE +
            sizeof(compressed) + 
            compressed + 
            sizeof(compressedBgSize) + 
            compressedBgSize +
            sizeof(uint32_t) +
            collectibles.size() * (sizeof(float) * 2 + sizeof(CollectibleType));

        auto levelPacket = new char[lvlSize];
        
        levelPacket[0] = Header::LEVEL; 

        *(uint32_t*)(levelPacket + 1) = compressed;
        char* dst = levelPacket + 5;

        memcpy(dst, compressedData.data(), compressed);

        *(uint32_t*)(levelPacket + 5 + compressed) = compressedBgSize;
        char* dstBg = levelPacket + 5 + compressed + 4;

        memcpy(dstBg, compressedBg.data(), compressedBgSize);

        auto i = HEADER_SIZE + sizeof(uint32_t) + compressed + sizeof(uint32_t) + compressedBgSize;

        *(uint32_t*)(levelPacket + i) = collectibles.size();
        i += 4;

        for (auto& coll : collectibles) {
            *(float*)(levelPacket + i) = coll.pos.x;
            i += 4;
            
            *(float*)(levelPacket + i) = coll.pos.y;
            i += 4;

            *(CollectibleType*)(levelPacket + i) = coll.type;
            i++;
        }

        sendPacketTo(levelPacket, lvlSize);
        
        auto& pos = srv.getLevel().getRandomSpawn();

        m_player = Player {
            .x = pos.x, 
            .y = pos.y, 
            .hp = 100, 
            .hat = hat
        };
        
        m_player.nickname = pack->username;

        AddPlayerPacket newPlayerPacket {
            .peerId = m_peer->connectID,
            .position = pos,
            .hat = hat,
        };
        
        std::copy(m_player.nickname.begin(), m_player.nickname.end(), newPlayerPacket.nickname);
        
        srv.broadcast((char*)&newPlayerPacket, sizeof(newPlayerPacket), true);

        // TODO: Replace with struct or smth better :P
        auto clientsCount = clients.size();

        auto playerPacketSize = HEADER_SIZE + 
        sizeof(clientsCount) + (sizeof(float) * 2 + sizeof(uint16_t) + MAX_NAME_LEN + sizeof(uint8_t) * 4 + sizeof(uint32_t)) * (clientsCount - 1);
        auto playersPacket = new char[playerPacketSize];

        playersPacket[0] = Header::ADDALLPLAYERS;
        playersPacket[1] = (uint8_t)(clients.size() - 1);
        auto packetIndex = 2;

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

                *(uint8_t*)(playersPacket + packetIndex) = client.m_player.hp;
                packetIndex++;
                *(uint8_t*)(playersPacket + packetIndex) = client.m_player.currentWeapon;
                packetIndex++;
                *(uint8_t*)(playersPacket + packetIndex) = client.m_player.hat;
                packetIndex++;
                *(uint8_t*)(playersPacket + packetIndex) = client.m_player.isDied;
                packetIndex++;
            } 
        }

        sendPacketTo(playersPacket, playerPacketSize);

        m_loggedIn = true;

        srv.sendServerMessage(std::format("{} joined the server", pack->username));

        delete[] playersPacket;
        delete[] levelPacket;
    } else {
        // Processing packets
        switch (header) {
            case Header::MOVE: {
                if (!m_player.isDied) {
                    bool shouldExclude = true;

                    MovePacket* pack = reinterpret_cast<MovePacket*>(packet->data);
                    auto x = pack->position.x;
                    auto y = pack->position.y;
                    
                    if (x < 0 || x >= WORLD_SIZE || y >= WORLD_SIZE || y < 0) {
                        auto& lvl = srv.getLevel();
                        auto& pos = lvl.getRandomSpawn();

                        m_player.x = pos.x;
                        m_player.y = pos.y;

                        shouldExclude = false;
                    } else {
                        m_player.x = x;
                        m_player.y = y;
                    }
                    
                    MovePacket movePacket {
                        .id = this->getID(),
                        .position = Vector2 { m_player.x, m_player.y }
                    };

                    if (shouldExclude) {
                        srv.broadcastWithExclude((char*)&movePacket, sizeof(movePacket), m_peer->connectID);
                    } else {
                        srv.broadcast((char*)&movePacket, sizeof(movePacket), false);
                    }
                }

                break;
            }

            case Header::SHOOT: {
                if (!m_player.isDied) {
                    if (m_player.reload.at(m_player.currentWeapon) <= 0) {
                        ShootPacket* pack = reinterpret_cast<ShootPacket*>(packet->data);

                        auto angle = pack->angle;
                        auto& level = srv.getLevel();

                        Vector2 gunPos = {weaponsSize.at(m_player.currentWeapon).x, 0};

                        float cosA = cosf(angle);
                        float sinA = sinf(angle);

                        Vector2 gunWorld = {
                            gunPos.x * cosA - gunPos.y * sinA,
                            gunPos.x * sinA + gunPos.y * cosA
                        };

                        Weapon& wpn = weapons.at(m_player.currentWeapon);
                        float angleDeg = angle * RAD2DEG;

                        int flip = !(angleDeg >= -90 && angleDeg < 90) ? -1 : 1;
                        if (m_player.currentWeapon == SHOTGUN) {
                            std::uniform_real_distribution<float> distr(-0.1f, 0.1f); 

                            for (int i = 0; i < 3; i++) {
                                auto angl = angle + distr(srv.getLevel().getGen());
                                cosA = cosf(angl);
                                sinA = sinf(angl);

                                Bullet bullet = Bullet {
                                    {gunWorld.x + 0.2f + m_player.x, gunWorld.y + m_player.y + 0.3f},   
                                    {cosA * wpn.bulletSpeed, sinA * wpn.bulletSpeed}, 
                                    static_cast<uint32_t>(level.bulletSize()),
                                    0,
                                    wpn.lifeTime, 
                                    m_peer->connectID,
                                    m_player.currentWeapon
                                };
                                    
                                level.addBullet(bullet);

                                sendBullet(bullet, angl);
                            }

                            m_player.reload.at(m_player.currentWeapon) = wpn.reloadTime;
                        } else {
                            Bullet bullet = Bullet {
                                {gunWorld.x + 0.2f + m_player.x, gunWorld.y + m_player.y + 0.3f},   
                                {cosf(angle) * wpn.bulletSpeed, sinf(angle) * wpn.bulletSpeed}, 
                                static_cast<uint32_t>(level.bulletSize()),
                                0,
                                wpn.lifeTime, 
                                m_peer->connectID,
                                m_player.currentWeapon
                            };
                                
                            level.addBullet(bullet);

                            sendBullet(bullet, angle);
                            m_player.reload.at(m_player.currentWeapon) = wpn.reloadTime;
                        }
                    }
                }

                break;
            }
            case Header::UPDATEWEAPON: {
                if (!m_player.isDied) {
                    UpdateWeaponPacket* pack = reinterpret_cast<UpdateWeaponPacket*>(packet->data);

                    auto index = pack->index;

                    if (index >= m_player.inventory.size() || index >= WEAPONS_COUNT || m_player.currentWeapon == index) {
                        return;
                    }

                    if (m_player.inventory.at(index) == false) {
                        for (int i = index; i < WEAPONS_COUNT; i++) {
                            if (m_player.inventory.at(i)) {
                                index = (WeaponType) i;
                                break;
                            }   
                        }
                    }
                    
                    if (m_player.inventory.at(index) == false) {
                        return;
                    }
                    auto weaponSize = HEADER_SIZE + sizeof(uint8_t) + sizeof(m_peer->connectID);

                    m_player.currentWeapon = index;
                    
                    UpdateWeaponPacket updateWeapon {
                        .id = getID(),
                        .index = index,
                    };

                    srv.broadcast((char*)&updateWeapon, sizeof(updateWeapon), false);
                }

                break;
            }

            case Header::UPDATEANGLE: {
                UpdateAnglePacket* pack = reinterpret_cast<UpdateAnglePacket*>(packet->data);

                UpdateAnglePacket anglePacket {
                    .id = getID(),
                    .angle = pack->angle
                };
                
                srv.broadcast((char*)&anglePacket, sizeof(anglePacket), false);

                break;
            }
            
            case Header::MESSAGE: {
                MessagePacket* pack = reinterpret_cast<MessagePacket*>(packet->data);
                srv.sendServerMessage(std::format("<{}> {}", m_player.nickname, pack->message));

                break;
            }

            case Header::THROWGRENADE: {
                if (!m_player.isDied && m_player.grenade != GRENADE_NONE) {
                    ThrowGrenadePacket* pack = reinterpret_cast<ThrowGrenadePacket*>(packet->data);
                    float angle = pack->angle;

                    auto& level = srv.getLevel();

                    auto& stats = grenades.at(m_player.grenade - 1);
                    
                    Grenade grenade = { 0 };
                    std::uniform_real_distribution<float> distr(-0.1f, 0.1f); 

                    angle += distr(srv.getLevel().getGen());
                    
                    grenade.pos = {m_player.x + 0.5f, m_player.y + 0.5f};
                    grenade.velocity = {stats.speed * cosf(angle), stats.speed * sinf(angle)};
                    grenade.id = level.grenadesSize();
                    grenade.grenadeId = m_player.grenade;
                    grenade.lifeTime = stats.lifeTime;
                    grenade.owner = m_peer->connectID;
                    
                    level.addGrenade(grenade);

                    m_player.grenade = GRENADE_NONE;
                    
                    AddGrenadePacket grenadePacket {
                        .angle = angle,
                        .id = grenade.id,
                        .position = grenade.pos,
                        .velocity = grenade.velocity
                    };

                    auto& srv = Server::get();

                    srv.broadcast((char*)&grenadePacket, sizeof(grenadePacket));
                }
                
                break;
            }

            case Header::REVIVE: {
                auto& pos = srv.getLevel().getRandomSpawn();
                RevivePacket revivePacket {
                    .id = getID(),
                    .position = pos
                };

                srv.broadcast((char*)&revivePacket, sizeof(revivePacket));

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