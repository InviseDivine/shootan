#include <Level.hpp>
#include <cstdint>
#include <iostream>
#include <random>
#include "Types.hpp"
#include "Weapons.hpp"
#include "Server.hpp"
#include <algorithm>
#include <fstream>
#include <Collisions.hpp>

// https://github.com/raysan5/raylib/blob/65abee1cbade6bf7edf55da6eb1eed6980aa754b/src/rshapes.c#L2267
bool CheckCollisionPointRec(RVector2 point, RRectangle rec) {
    bool collision = false;

    if ((point.x >= rec.x) && (point.x < (rec.x + rec.width)) && (point.y >= rec.y) && (point.y < (rec.y + rec.height))) collision = true;

    return collision;
}

// https://github.com/raysan5/raylib/blob/65abee1cbade6bf7edf55da6eb1eed6980aa754b/src/rshapes.c#L2329
bool CheckCollisionRecs(RRectangle rec1, RRectangle rec2) {
    bool collision = false;

    if ((rec1.x < (rec2.x + rec2.width) && (rec1.x + rec1.width) > rec2.x) &&
        (rec1.y < (rec2.y + rec2.height) && (rec1.y + rec1.height) > rec2.y)) collision = true;

    return collision;
}

void sendHpPacket(uint32_t id, Player& plr, Server& srv) {
    auto size = HEADER_SIZE + sizeof(id) + sizeof(plr.hp);
    auto sendHp = new char[size];

    sendHp[0] = SETHP;

    *(uint32_t*)(sendHp + HEADER_SIZE) = id;
    *(int*)(sendHp + HEADER_SIZE + 4) = plr.hp;

    srv.broadcast(sendHp, size);

    delete [] sendHp;
}

inline void removeBulletPacket(uint32_t index, Server& srv) {
    auto removeBullet = new char[HEADER_SIZE + sizeof(index)];

    removeBullet[0] = REMOVEBULLET;

    *(uint32_t*)(removeBullet + HEADER_SIZE) = index;
    
    srv.broadcast(removeBullet, HEADER_SIZE + sizeof(index));
    
    delete [] removeBullet;
}

Level::Level() {
    std::random_device rd;
    m_gen = std::mt19937(rd());
    m_scoreLimit = 10;
}

void Level::restartGame() {
    auto& srv = Server::get();
    
    // TODO: one packet for update
    for (auto& [id, plr] : srv.getClients()) {
        plr.m_player.score = 0;

        // pos
        auto& pos = getRandomSpawn();

        plr.m_player.x = pos.x;
        plr.m_player.y = pos.y;

        auto moveSize = HEADER_SIZE + sizeof(float) * 2 + sizeof(id);
        auto movePlrPacket = new char[moveSize];
        
        movePlrPacket[0] = MOVE;
        
        *(uint32_t*)(movePlrPacket + 1) = id;
        *(float*)(movePlrPacket + 5) = plr.m_player.x;
        *(float*)(movePlrPacket + 9) = plr.m_player.y;

        srv.broadcast(movePlrPacket, moveSize);
        
        delete [] movePlrPacket;

        plr.m_player.inventory = {true};
        plr.m_player.reload = 0;
        plr.m_player.hp = 100;
        plr.m_player.currentWeapon = 0;

        auto endSize = HEADER_SIZE + sizeof(uint32_t);
        auto endPacket = new char[endSize];

        endPacket[0] = Header::ROUNDEND;

        *(uint32_t*)(endPacket + HEADER_SIZE) = 0;

        plr.sendPacketTo(endPacket, endSize);
        delete [] endPacket;
    }
}

void Level::update() {
    auto& srv = Server::get();
    
    if (m_ticksEnd > 0) {
        m_ticksEnd -= 1.f;
    } else {
        if (m_roundEnd) {
            restartGame();
            m_roundEnd = false;
        }
    }
    
    // Collectibles
    for (auto& coll : m_collectibles) {
        for (auto& [id, plr] : srv.getClients()) {
            if (coll.respawnTime > 0) {
                if (coll.isSent) {
                    // TODO: Put it to function
                    auto size = HEADER_SIZE + sizeof(float) * 2 + sizeof(Collectible);
                    auto sendCollectible = new char[size];

                    sendCollectible[0] = UPDATECOLLECTIBLE;

                    *(float*)(sendCollectible + HEADER_SIZE) = coll.pos.x;
                    *(float*)(sendCollectible + HEADER_SIZE + 4) = coll.pos.y;
                    *(Collectibles*)(sendCollectible + HEADER_SIZE + 8) = NONE;

                    srv.broadcast(sendCollectible, size);
                    
                    delete [] sendCollectible;
                    coll.isSent = false;
                }
                
                coll.respawnTime -= 1.f;
            } else {
                if (CheckCollisionPointRec({coll.pos.x, coll.pos.y}, {plr.m_player.x, plr.m_player.y, 1.f, 1.f})) {
                    if (coll.type == MEDKIT) {
                        if (plr.m_player.hp < 100) {
                            plr.m_player.hp += 15;
                            if (plr.m_player.hp > 100) plr.m_player.hp = 100;
                            
                            sendHpPacket(id, plr.m_player, srv);
                        } else {
                            continue;
                        }
                    } else {
                        // TODO: it can be better?
                        switch (coll.type) {
                            case SHOTGUN_COLLECT: {
                                auto& inv = plr.m_player.inventory;

                                if (inv.at(SHOTGUN) == false) {
                                    auto weaponShotgun = new char[HEADER_SIZE + 1];
                                    weaponShotgun[0] = ADDWEAPON;
                                    weaponShotgun[1] = SHOTGUN;

                                    plr.m_player.inventory.at(SHOTGUN) = true;

                                    plr.sendPacketTo(weaponShotgun, 2);
                                } else {
                                    continue;
                                }
                                
                                break;
                            }
                            
                            case SNIPER_COLLECT: {
                                auto& inv = plr.m_player.inventory;

                                if (inv.at(SNIPER_RIFLE) == false) {
                                    auto weaponShotgun = new char[HEADER_SIZE + 1];
                                    weaponShotgun[0] = ADDWEAPON;
                                    weaponShotgun[1] = SNIPER_RIFLE;

                                    plr.m_player.inventory.at(SNIPER_RIFLE) = true;

                                    plr.sendPacketTo(weaponShotgun, 2);
                                } else {
                                    continue;
                                }
                                
                                break;
                            }

                            case GRENADE_COLLECT : {
                                auto& grenade = plr.m_player.grenade;
                                
                                std::cout << "Grenade: " << grenade << std::endl;

                                if (grenade == GRENADE_NONE) {
                                    auto grenadePacket = new char[HEADER_SIZE + 1];
                                    grenadePacket[0] = ADDGRENADE;
                                    grenadePacket[1] = COMMON;

                                    plr.sendPacketTo(grenadePacket, 2);

                                    plr.m_player.grenade = COMMON;
                                } else {
                                    continue;
                                }

                                break;     
                            }
                            default: break;
                        }
                    }

                    coll.respawnTime = 600.f;
                }

                if (!coll.isSent) {
                    // TODO: Put it to function
                    auto size = HEADER_SIZE + sizeof(float) * 2 + sizeof(Collectible);
                    auto sendCollectible = new char[size];

                    sendCollectible[0] = UPDATECOLLECTIBLE;

                    *(float*)(sendCollectible + HEADER_SIZE) = coll.pos.x;
                    *(float*)(sendCollectible + HEADER_SIZE + 4) = coll.pos.y;
                    *(Collectibles*)(sendCollectible + HEADER_SIZE + 8) = coll.type;

                    srv.broadcast(sendCollectible, size);
                    
                    delete [] sendCollectible;
                    coll.isSent = true;
                }
            }
        }
    }

    // Bullets
    for (auto& bullet : m_bullets) {
        for (auto& client : srv.getClients()) {       
            auto& plr = client.second.m_player;
            Weapon& wpn = weapons.at(bullet.weaponId);
            
            if (bullet.owner != client.first && CheckCollisionPointRec(
                {bullet.pos.x, bullet.pos.y}, 
                {client.second.m_player.x, client.second.m_player.y, 1.f, 1.f}) 
            ) {
                auto& owner = *srv.getClients().find(bullet.owner);

                client.second.m_player.hp -= wpn.damage;
                auto dmgSize = HEADER_SIZE + sizeof(uint32_t) + sizeof(int);
                auto dmgPacket = new char[dmgSize];

                dmgPacket[0] = Header::DAMAGE;

                *(uint32_t*)(dmgPacket + HEADER_SIZE) = client.first;
                *(int*)(dmgPacket + HEADER_SIZE + 4) = wpn.damage;

                owner.second.sendPacketTo(dmgPacket, dmgSize);

                delete [] dmgPacket;

                if (client.second.m_player.hp <= 0) {
                    plr.hp = 100;    

                    sendHpPacket(client.first, client.second.m_player, srv);

                    // score
                    if (!m_roundEnd) {
                        owner.second.m_player.score++;
                        srv.sendServerMessage(std::format("{} killed by {}", client.second.m_player.nickname, owner.second.m_player.nickname));

                        auto scoreSize = HEADER_SIZE + sizeof(uint32_t) + sizeof(int);
                        auto scorePacket = new char[scoreSize];

                        scorePacket[0] = Header::SETSCORE;

                        *(uint32_t*)(scorePacket + HEADER_SIZE) = bullet.owner;
                        *(int*)(scorePacket + HEADER_SIZE + 4) = owner.second.m_player.score;

                        srv.broadcast(scorePacket, scoreSize);
                        
                        delete [] scorePacket;
                    }
                    
                    // round end
                    if (owner.second.m_player.score >= m_scoreLimit) {
                        auto endSize = HEADER_SIZE + sizeof(uint32_t);
                        auto endPacket = new char[endSize];

                        endPacket[0] = Header::ROUNDEND;

                        *(uint32_t*)(endPacket + HEADER_SIZE) = bullet.owner;

                        srv.broadcast(endPacket, endSize);
                        srv.sendServerMessage(std::format("{} wins!", owner.second.m_player.nickname));

                        m_roundEnd = true;
                        m_ticksEnd = 500.f;
                        
                        delete [] endPacket;
                    }

                    // respawn pos
                    auto moveSize = HEADER_SIZE + sizeof(float) * 2 + sizeof(client.first);
                    auto movePlrPacket = new char[moveSize];
                    
                    movePlrPacket[0] = MOVE;
                    
                    auto& pos = getRandomSpawn();
                    
                    plr.x = pos.x;
                    plr.y = pos.y;
                    
                    *(uint32_t*)(movePlrPacket + 1) = client.first;
                    *(float*)(movePlrPacket + 5) = pos.x;
                    *(float*)(movePlrPacket + 9) = pos.y;

                    srv.broadcast(movePlrPacket, moveSize);

                    delete [] movePlrPacket;
                } else {
                    sendHpPacket(client.first, client.second.m_player, srv);
                }
                
                removeBulletPacket(bullet.id, srv);

                std::erase_if(m_bullets, [&bullet](Bullet blt) { 
                    return blt.id == bullet.id;
                });
                continue;
            } else if (bullet.lifeTime <= 0 || (GetBlock(bullet.pos.x, bullet.pos.y) && GetBlock(bullet.pos.x, bullet.pos.y) != LADDER)) {
                removeBulletPacket(bullet.id, srv);

                std::erase_if(m_bullets, [&bullet](Bullet blt) { 
                    return blt.id == bullet.id;
                });                
                continue;
            }     
        }

        // std::cout << bullet.id << " " << bullet.lifeTime << std::endl;

        bullet.pos.x += bullet.velocity.x;
        bullet.pos.y += bullet.velocity.y;
        bullet.lifeTime -= 1.f;   
    }

    for (auto& [_, plr] : srv.getClients()) {
        if (plr.m_player.reload > 0) {
            plr.m_player.reload -= 1.f;
        }
    }

    // TODO: Rewrite
    for (auto& grenade : m_grenades) {
        grenade.velocity.y += 0.02f;
        auto& stats = grenades.at(grenade.grenadeId - 1);

        float prevX = grenade.velocity.x;
        float prevY = grenade.velocity.y;

        int blocksAroundCount = 10;
        
        std::vector<RVector2> blocksAroundArr = getBlocksAround({grenade.pos.x, grenade.pos.y}, stats.burstRadius);
        
        RRectangle grenadeBox = {grenade.pos.x, grenade.pos.y, 0.5f, 0.5f};

        float x = grenade.velocity.x;
        // Check for X collision
        for (int i = 0; i < blocksAroundArr.size(); i++) {
            x = ClipX(RRectangle{blocksAroundArr.at(i).x, blocksAroundArr.at(i).y, 1.0f, 1.0f}, grenadeBox, x);
        }
        auto tempX = grenade.pos.x + x;

        if (tempX >= 0 && tempX < WORLD_SIZE - 1) {
            grenadeBox.x = grenade.pos.x = tempX;
        }
        
        float y = grenade.velocity.y;
        // Check for Y collision
        for (int i = 0; i < blocksAroundArr.size(); i++) {
            y = ClipY(RRectangle{blocksAroundArr.at(i).x, blocksAroundArr.at(i).y, 1.0f, 1.0f}, grenadeBox, y);
        }
        grenade.pos.y += y;

        // printf("%f %f \n", x, y);

        // Stop motion on collision
        if (prevX != x) grenade.velocity.x *= -1;
        // if (prevY != y) grenade.velocity.y *= -1;

        if (grenade.velocity.x > -0.05f && grenade.velocity.x < 0.05f) grenade.velocity.x = 0;
        grenade.velocity.x *= 0.95f;
        grenade.velocity.y *= 0.99f;   

        grenade.lifeTime -= 1.f;

        if (grenade.lifeTime <= 0) {
            auto& stats = grenades.at(grenade.grenadeId - 1);

            int minY = grenade.pos.y - stats.burstRadius;
            int minX = grenade.pos.x - stats.burstRadius;

            int maxY = grenade.pos.y + stats.burstRadius;
            int maxX = grenade.pos.x + stats.burstRadius;

            for (int y = minY; y < maxY; y++) {
                for (int x = minX; x < maxX; x++) {
                    for (auto& [id, client] : srv.getClients()) {
                        if ((int)client.m_player.x == x && (int)client.m_player.y == y) {
                            // TODO: Remove repeat code
                            client.m_player.hp -= stats.damage;
                            auto& owner = *srv.getClients().find(grenade.owner);

                            auto dmgSize = HEADER_SIZE + sizeof(uint32_t) + sizeof(int);
                            auto dmgPacket = new char[dmgSize];

                            dmgPacket[0] = Header::DAMAGE;

                            *(uint32_t*)(dmgPacket + HEADER_SIZE) = id;
                            *(int*)(dmgPacket + HEADER_SIZE + 4) = stats.damage;

                            owner.second.sendPacketTo(dmgPacket, dmgSize);

                            delete [] dmgPacket;

                            if (client.m_player.hp <= 0) {
                                client.m_player.hp = 100;    

                                sendHpPacket(id, client.m_player, srv);

                                // score
                                if (grenade.owner != id) {
                                    if (!m_roundEnd) {
                                        owner.second.m_player.score++;
                                        srv.sendServerMessage(std::format("{} killed by {}", client.m_player.nickname, owner.second.m_player.nickname));

                                        auto scoreSize = HEADER_SIZE + sizeof(uint32_t) + sizeof(int);
                                        auto scorePacket = new char[scoreSize];

                                        scorePacket[0] = Header::SETSCORE;

                                        *(uint32_t*)(scorePacket + HEADER_SIZE) = grenade.owner;
                                        *(int*)(scorePacket + HEADER_SIZE + 4) = owner.second.m_player.score;

                                        srv.broadcast(scorePacket, scoreSize);
                                        
                                        delete [] scorePacket;
                                    }
                                    
                                    // round end
                                    if (owner.second.m_player.score >= m_scoreLimit) {
                                        auto endSize = HEADER_SIZE + sizeof(uint32_t);
                                        auto endPacket = new char[endSize];

                                        endPacket[0] = Header::ROUNDEND;

                                        *(uint32_t*)(endPacket + HEADER_SIZE) = grenade.owner;

                                        srv.broadcast(endPacket, endSize);
                                        srv.sendServerMessage(std::format("{} wins!", owner.second.m_player.nickname));

                                        m_roundEnd = true;
                                        m_ticksEnd = 500.f;
                                        
                                        delete [] endPacket;
                                    }
                                }

                                // respawn pos
                                auto moveSize = HEADER_SIZE + sizeof(float) * 2 + sizeof(id);
                                auto movePlrPacket = new char[moveSize];
                                
                                movePlrPacket[0] = MOVE;
                                
                                auto& pos = getRandomSpawn();
                                
                                client.m_player.x = pos.x;
                                client.m_player.y = pos.y;
                                
                                *(uint32_t*)(movePlrPacket + 1) = id;
                                *(float*)(movePlrPacket + 5) = pos.x;
                                *(float*)(movePlrPacket + 9) = pos.y;

                                srv.broadcast(movePlrPacket, moveSize);

                                delete [] movePlrPacket;
                            } else {
                                sendHpPacket(id, client.m_player, srv);
                            }                        
                        }
                    }
                }
            }

            auto removeGrenade = HEADER_SIZE + sizeof(grenade.id);
            auto removePacket = new char[removeGrenade];
            
            removePacket[0] = REMOVEGRENADE;
            
            *(uint32_t*)(removePacket + 1) = grenade.id;

            srv.broadcast(removePacket, removeGrenade);

            delete [] removePacket;

            auto test = std::erase_if(m_grenades, [&grenade](Grenade grnd) { 
                return grenade.id == grnd.id;
            }); 

            printf("%d\n", test);
        }
    }
}

void Level::read(const std::string& filepath) {
    std::ifstream world(filepath, std::ios::binary);

    if (world) {
        world.read(reinterpret_cast<char*>(m_world.data()),
            WORLD_SIZE * WORLD_SIZE);

        world.read(reinterpret_cast<char*>(m_background.data()),
            WORLD_SIZE * WORLD_SIZE);
            
        uint32_t collSize = 0;

        world.read(reinterpret_cast<char*>(&collSize), sizeof(collSize));

        m_collectibles.resize(collSize);

        world.read(reinterpret_cast<char*>(m_collectibles.data()), collSize * sizeof(Collectible));

        uint32_t spawnpointSize = 0;

        world.read(reinterpret_cast<char*>(&spawnpointSize), sizeof(spawnpointSize));

        m_respawnPoints.resize(spawnpointSize);

        world.read(reinterpret_cast<char*>(m_respawnPoints.data()), spawnpointSize * sizeof(RVector2));

        world.close();
    }   
}

std::vector<RVector2> Level::getBlocksAround(RVector2 pos, int radius) {
    std::vector<RVector2> blocksAround;

    int blocksAroundCount = 0;
    for (int yy = (int)pos.y - radius; yy <= (int)(pos.y + radius); yy++) {
        for (int xx = (int)pos.x - radius; xx <= (int)(pos.x + radius); xx++) {
            if (GetBlock(xx, yy)) {
                if (GetBlock(xx, yy) == LADDER) {
                    continue;
                }
                
                if (blocksAroundCount >= 10) {
                    break;
                } 

                blocksAround.push_back(RVector2{(float)xx, (float)yy});
                blocksAroundCount++;
            }
        }
    }

    return blocksAround;
}