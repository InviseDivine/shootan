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
bool CheckCollisionPointRec(Vector2 point, Rectangle rec) {
    bool collision = false;

    if ((point.x >= rec.x) && (point.x < (rec.x + rec.width)) && (point.y >= rec.y) && (point.y < (rec.y + rec.height))) collision = true;

    return collision;
}

// https://github.com/raysan5/raylib/blob/65abee1cbade6bf7edf55da6eb1eed6980aa754b/src/rshapes.c#L2329
bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2) {
    bool collision = false;

    if ((rec1.x < (rec2.x + rec2.width) && (rec1.x + rec1.width) > rec2.x) &&
        (rec1.y < (rec2.y + rec2.height) && (rec1.y + rec1.height) > rec2.y)) collision = true;

    return collision;
}

void sendHpPacket(uint32_t id, Player& plr, Server& srv) {
    SetHpPacket hpPacket {
        .id = id,
        .hp = plr.hp,
    };
    
    srv.broadcast((char*)&hpPacket, sizeof(hpPacket));
}

inline void removeBulletPacket(uint32_t index, Server& srv) {
    RemoveBulletPacket removePacket {
        .bulletId = index,
    };
    
    srv.broadcast((char*)&removePacket, sizeof(removePacket));
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

        auto& pos = getRandomSpawn();

        plr.m_player.x = pos.x;
        plr.m_player.y = pos.y;

        MovePacket movePacket {
            .position = pos
        };

        srv.broadcast((char*)&movePacket, sizeof(movePacket));
        
        plr.m_player.inventory = {true};
        plr.m_player.reload = {};
        plr.m_player.hp = 100;
        plr.m_player.currentWeapon = (WeaponType) 0;

        RoundEndPacket endPacket {
            .winner = 0
        };

        srv.broadcast((char*)&endPacket, sizeof(endPacket));
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
                    UpdateCollectiblePacket updatePacket {
                        .position = coll.pos,
                        .type = CollectibleType::NONE
                    };
                    
                    srv.broadcast((char*)&updatePacket, sizeof(updatePacket));
                    coll.isSent = false;
                }
                
                coll.respawnTime -= 1.f;
            } else {
                bool gotCollectible = CheckCollisionPointRec({coll.pos.x, coll.pos.y}, {plr.m_player.x, plr.m_player.y, 1.f, 1.f});
                if (!plr.m_player.isDied && gotCollectible) {
                    if (coll.type == MEDKIT) {
                        if (plr.m_player.hp < 100) {
                            plr.m_player.hp += 15;
                            if (plr.m_player.hp > 100) plr.m_player.hp = 100;
                            
                            sendHpPacket(id, plr.m_player, srv);
                        } else {
                            continue;
                        }
                    } else {
                        // NOTE: can it be better?
                        switch (coll.type) {
                            case SHOTGUN_COLLECT: {
                                auto& inv = plr.m_player.inventory;

                                if (inv.at(WeaponType::SHOTGUN) == false) {
                                    AddWeaponPacket addPacket {
                                        .type = WeaponType::SHOTGUN
                                    };

                                    plr.m_player.inventory.at(WeaponType::SHOTGUN) = true;

                                    plr.sendPacketTo((char*)&addPacket, sizeof(addPacket));
                                } else {
                                    continue;
                                }
                                
                                break;
                            }
                            
                            case SNIPER_COLLECT: {
                                auto& inv = plr.m_player.inventory;

                                if (inv.at(SNIPER_RIFLE) == false) {
                                    AddWeaponPacket addPacket {
                                        .type = SNIPER_RIFLE
                                    };

                                    plr.m_player.inventory.at(SNIPER_RIFLE) = true;

                                    plr.sendPacketTo((char*)&addPacket, sizeof(addPacket));
                                } else {
                                    continue;
                                }
                                
                                break;
                            }

                            case GRENADE_COLLECT : {
                                auto& grenade = plr.m_player.grenade;
                                
                                if (grenade == GRENADE_NONE) {
                                    AddGrenadeInventoryPacket addPacket {
                                        .type = GrenadeType::GRENADE_COMMON
                                    };

                                    plr.m_player.grenade = GrenadeType::GRENADE_COMMON;

                                    plr.sendPacketTo((char*)&addPacket, sizeof(addPacket));
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
                    UpdateCollectiblePacket updatePacket {
                        .position = coll.pos,
                        .type = coll.type
                    };

                    srv.broadcast((char*)&updatePacket, sizeof(updatePacket));
                    
                    coll.isSent = true;
                }
            }
        }
    }

    // Bullets
    // TODO: Remove repeat code
    for (auto& bullet : m_bullets) {
        for (auto& client : srv.getClients()) {       
            // TODO: make functions to individual checks
            // ig it will be better

            auto& plr = client.second.m_player;
            auto id = client.first;
            
            Weapon& wpn = weapons.at(bullet.weaponId);
            
            bool isHit = CheckCollisionPointRec({bullet.pos.x, bullet.pos.y}, {plr.x, plr.y, 1.f, 1.f});
            bool notOwner = bullet.owner != client.first;

            if (notOwner && !plr.isDied && isHit) {
                auto& owner = *srv.getClients().find(bullet.owner);

                plr.hp -= wpn.damage;

                DamagePacket damagePacket {
                    .damageReceiver = id,
                    .damage = wpn.damage
                };

                owner.second.sendPacketTo((char*)&damagePacket, sizeof(damagePacket));

                if (plr.hp <= 0) {
                    // plr.hp = 100;    

                    // sendHpPacket(client.first, client.second.m_player, srv);
                    plr.isDied = true;    

                    // score
                    if (!m_roundEnd) {
                        owner.second.m_player.score++;
                        srv.sendServerMessage(std::format("{} killed by {}", plr.nickname, owner.second.m_player.nickname));

                        ScorePacket scorePacket {
                            .killerId = bullet.owner,
                            .deadId = id,
                            .score = owner.second.m_player.score
                        };

                        // *(uint32_t*)(scorePacket + HEADER_SIZE) = bullet.owner;
                        // *(uint32_t*)(scorePacket + HEADER_SIZE + 4) = client.first;
                        // *(int*)(scorePacket + HEADER_SIZE + 8) = owner.second.m_player.score;

                        srv.broadcast((char*)&scorePacket, sizeof(scorePacket));
                    }
                    
                    // round end
                    if (owner.second.m_player.score >= m_scoreLimit) {
                        RoundEndPacket endPacket {
                            .winner = bullet.owner
                        };

                        srv.broadcast((char*)&endPacket, sizeof(endPacket));
                        srv.sendServerMessage(std::format("{} wins!", owner.second.m_player.nickname));

                        m_roundEnd = true;
                        m_ticksEnd = 10 * TICKS;
                    }
                } else {
                    sendHpPacket(client.first, client.second.m_player, srv);
                }
                
                removeBulletPacket(bullet.id, srv);

                std::erase_if(m_bullets, [&bullet](Bullet blt) { 
                    return blt.id == bullet.id;
                });
                continue;
            } else {
                bool hitBlock = GetBlock(bullet.pos.x, bullet.pos.y) && GetBlock(bullet.pos.x, bullet.pos.y) != LADDER;
                if (bullet.lifeTime <= 0 || hitBlock) {
                    removeBulletPacket(bullet.id, srv);

                    std::erase_if(m_bullets, [&bullet](Bullet blt) { 
                        return blt.id == bullet.id;
                    });                
                    continue;   
                }
            }     
        }

        bullet.pos.x += bullet.velocity.x;
        bullet.pos.y += bullet.velocity.y;
        bullet.lifeTime -= 1.f;   
    }

    for (auto& [_, plr] : srv.getClients()) {
        for (int i = 0; i < plr.m_player.reload.size(); i++) {
            auto& reload = plr.m_player.reload.at(i);

            if (reload > 0) {
                reload -= 1.f;
            }
        }
    }

    // TODO: Rewrite
    for (auto& grenade : m_grenades) {
        grenade.velocity.y += 0.02f;
        auto& stats = grenades.at(grenade.grenadeId - 1);

        float prevX = grenade.velocity.x;
        float prevY = grenade.velocity.y;

        int blocksAroundCount = 10;
        
        std::vector<Vector2> blocksAroundArr = getBlocksAround({grenade.pos.x, grenade.pos.y}, stats.burstRadius);
        
        Rectangle grenadeBox = {grenade.pos.x, grenade.pos.y, 0.5f, 0.5f};

        float x = grenade.velocity.x;
        // Check for X collision
        for (int i = 0; i < blocksAroundArr.size(); i++) {
            x = ClipX(Rectangle {blocksAroundArr.at(i).x, blocksAroundArr.at(i).y, 1.0f, 1.0f}, grenadeBox, x);
        }
        auto tempX = grenade.pos.x + x;

        if (tempX >= 0 && tempX < WORLD_SIZE - 1) {
            grenadeBox.x = grenade.pos.x = tempX;
        }
        
        float y = grenade.velocity.y;
        // Check for Y collision
        for (int i = 0; i < blocksAroundArr.size(); i++) {
            y = ClipY(Rectangle {blocksAroundArr.at(i).x, blocksAroundArr.at(i).y, 1.0f, 1.0f}, grenadeBox, y);
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
            auto& grenadeStats = grenades.at(grenade.grenadeId - 1);

            int minY = grenade.pos.y - stats.burstRadius;
            int minX = grenade.pos.x - stats.burstRadius;

            int maxY = grenade.pos.y + stats.burstRadius;
            int maxX = grenade.pos.x + stats.burstRadius;

            for (int y = minY; y < maxY; y++) {
                for (int x = minX; x < maxX; x++) {
                    for (auto& [id, client] : srv.getClients()) {
                        bool inGrenadeRadius = (int)client.m_player.x == x && (int)client.m_player.y == y;
                        if (!client.m_player.isDied && inGrenadeRadius) {
                            // TODO: Remove repeat code
                            client.m_player.hp -= stats.damage;
                            auto& owner = *srv.getClients().find(grenade.owner);

                            DamagePacket damagePacket {
                                .damageReceiver = id,
                                .damage = grenadeStats.damage
                            };

                            owner.second.sendPacketTo((char*)&damagePacket, sizeof(damagePacket));

                            if (client.m_player.hp <= 0) {
                                client.m_player.isDied = true;    

                                // score
                                if (grenade.owner != id) {
                                    if (!m_roundEnd) {
                                        owner.second.m_player.score++;
                                        srv.sendServerMessage(std::format("{} killed by {}", client.m_player.nickname, owner.second.m_player.nickname));
                                        
                                        ScorePacket scorePacket {
                                            .killerId = grenade.owner,
                                            .deadId = client.getID(),
                                            .score = owner.second.m_player.score
                                        };

                                        srv.broadcast((char*)&scorePacket, sizeof(scorePacket));
                                    }
                                    
                                    // round end
                                    if (owner.second.m_player.score >= m_scoreLimit) {
                                        RoundEndPacket endPacket {
                                            .winner = grenade.owner
                                        };

                                        srv.broadcast((char*)&endPacket, sizeof(endPacket));
                                        srv.sendServerMessage(std::format("{} wins!", owner.second.m_player.nickname));

                                        m_roundEnd = true;
                                        m_ticksEnd = 500.f;
                                    }
                                } else {
                                    if (!m_roundEnd) {
                                        if (owner.second.m_player.score > 0) {
                                            owner.second.m_player.score--;
                                        }
                                        
                                        srv.sendServerMessage(std::format("{} killed himself with grenade", owner.second.m_player.nickname));
                                        
                                        ScorePacket scorePacket {
                                            .killerId = grenade.owner,
                                            .deadId = grenade.owner,
                                            .score = owner.second.m_player.score
                                        };

                                        srv.broadcast((char*)&scorePacket, sizeof(scorePacket));
                                    }
                                }

                                // respawn pos
                                // auto moveSize = HEADER_SIZE + sizeof(float) * 2 + sizeof(id);
                                // auto movePlrPacket = new char[moveSize];
                                
                                // movePlrPacket[0] = MOVE;
                                
                                // auto& pos = getRandomSpawn();
                                
                                // client.m_player.x = pos.x;
                                // client.m_player.y = pos.y;
                                
                                // *(uint32_t*)(movePlrPacket + 1) = id;
                                // *(float*)(movePlrPacket + 5) = pos.x;
                                // *(float*)(movePlrPacket + 9) = pos.y;

                                // srv.broadcast(movePlrPacket, moveSize);

                                // delete [] movePlrPacket;
                            } else {
                                sendHpPacket(client.getID(), client.m_player, srv);
                            }                        
                        }
                    }
                }
            }

            RemoveGrenadePacket removePacket {
                .grenadeId = grenade.id
            };

            srv.broadcast((char*)&removePacket, sizeof(removePacket));

            std::erase_if(m_grenades, [&grenade](Grenade secondGrenade) { 
                return grenade.id == secondGrenade.id;
            }); 
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

        if (spawnpointSize == 0) std::runtime_error("Spawnpoints size is zero!");

        m_respawnPoints.resize(spawnpointSize);

        world.read(reinterpret_cast<char*>(m_respawnPoints.data()), spawnpointSize * sizeof(Vector2));

        world.close();
    } else {
        throw std::runtime_error("Couldn't open world data!");
    }
}

std::vector<Vector2> Level::getBlocksAround(Vector2 pos, int radius) {
    std::vector<Vector2> blocksAround;

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

                blocksAround.push_back(Vector2 {(float)xx, (float)yy});
                blocksAroundCount++;
            }
        }
    }

    return blocksAround;
}