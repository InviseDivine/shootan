#include <Level.hpp>
#include <cstdint>
#include <iostream>
#include "Server.hpp"
#include "Types.hpp"
#include "Weapons.hpp"

// https://github.com/raysan5/raylib/blob/65abee1cbade6bf7edf55da6eb1eed6980aa754b/src/rshapes.c#L2267
bool CheckCollisionPointRec(RVector2 point, RRectangle rec) {
    bool collision = false;

    if ((point.x >= rec.x) && (point.x < (rec.x + rec.width)) && (point.y >= rec.y) && (point.y < (rec.y + rec.height))) collision = true;

    return collision;
}

// https://github.com/raysan5/raylib/blob/65abee1cbade6bf7edf55da6eb1eed6980aa754b/src/rshapes.c#L2329
bool CheckCollisionRecs(RRectangle rec1, RRectangle rec2)
{
    bool collision = false;

    if ((rec1.x < (rec2.x + rec2.width) && (rec1.x + rec1.width) > rec2.x) &&
        (rec1.y < (rec2.y + rec2.height) && (rec1.y + rec1.height) > rec2.y)) collision = true;

    return collision;
}

inline void removeBulletPacket(uint32_t index, Server& srv) {
    auto removeBullet = new char[HEADER_SIZE + sizeof(index)];

    removeBullet[0] = REMOVEBULLET;

    *(uint32_t*)(removeBullet + HEADER_SIZE) = index;
    
    srv.broadcast(removeBullet, HEADER_SIZE + sizeof(index));
    
    delete [] removeBullet;
}
void Level::update() {
    auto& srv = Server::get();

    for (auto& coll : m_collectibles) {
        for (auto& [_, plr] : srv.getClients()) {
            if (coll.respawnTime > 0) {
                coll.respawnTime -= 10.f;
            } else if (CheckCollisionPointRec({coll.pos.x, coll.pos.y}, {plr.m_player.x, plr.m_player.y, 1.f, 1.f})) {
                coll.respawnTime = 600.f;

                if (coll.type == MEDKIT) {
                    plr.m_player.hp += 15.f;
                } else {
                    // ..
                }
            }
        }
    }

    for (auto& bullet : m_bullets) {
        for (auto& client : srv.getClients()) {       
            auto& plr = client.second.m_player;
            Weapon& wpn = weapons.at(bullet.weaponId);
            
            if (CheckCollisionPointRec(
                {bullet.pos.x, bullet.pos.y}, 
                {client.second.m_player.x, client.second.m_player.y, 1.f, 1.f}) 
                && bullet.owner != client.first
            ) {
                client.second.m_player.hp -= wpn.damage;
                
                auto owner = srv.getClients().find(bullet.owner);
                owner->second.m_player.score++;
                
                if (client.second.m_player.hp <= 0) {
                    plr.hp = 100;    
                    
                    auto moveSize = HEADER_SIZE + sizeof(float) * 2 + sizeof(client.first);
                    auto movePlrPacket = new char[moveSize];
                    
                    movePlrPacket[0] = MOVE;
                    
                    *(uint32_t*)(movePlrPacket + 1) = client.first;
                    *(float*)(movePlrPacket + 5) = 1;
                    *(float*)(movePlrPacket + 9) = 60;

                    srv.broadcast(movePlrPacket, moveSize);

                    delete [] movePlrPacket;
                }
                
                removeBulletPacket(bullet.id, srv);
                std::erase_if(m_bullets, [&bullet](Bullet blt) { 
                    return blt.id == bullet.id;
                });

                continue;
            } else if (bullet.lifeTime <= 0 || GetBlock(bullet.pos.x, bullet.pos.y)) {
                std::erase_if(m_bullets, [&bullet](Bullet blt) { 
                    return blt.id == bullet.id;
                });
                
                removeBulletPacket(bullet.id, srv);
                
                continue;
            }     
        }

        bullet.pos.x += bullet.velocity.x;
        bullet.pos.y += bullet.velocity.y;
        bullet.lifeTime -= 1.f;   
    }
}