#include <Level.hpp>
#include <cstdint>
#include "Server.hpp"
#include "Types.hpp"
#include "Weapons.hpp"

bool CheckCollisionPointRec(RVector2 point, RRectangle rec) {
    bool collision = false;

    if ((point.x >= rec.x) && (point.x < (rec.x + rec.width)) && (point.y >= rec.y) && (point.y < (rec.y + rec.height))) collision = true;

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

    for (auto& bullet : m_bullets) {
        Weapon& wpn = weapons.at(bullet.weaponId);
        
        for (auto client : srv.getClients()) {        
            if (CheckCollisionPointRec(
                {bullet.pos.x, bullet.pos.y}, 
                {client.second.m_player.x, client.second.m_player.y, 1.f, 1.f}) 
                && bullet.owner != client.first
             ) {
                
                client.second.m_player.hp -= wpn.damage;
                
                std::erase_if(m_bullets, [&bullet](Bullet blt) { 
                    return blt.id == bullet.id;
                });
                
                removeBulletPacket(bullet.id, srv);
            } else if (bullet.lifeTime <= 0 || GetBlock(bullet.pos.y, bullet.pos.y)) { // TODO: Fix GetBlock (smth wrong with float to int conversion i think)
                std::erase_if(m_bullets, [&bullet](Bullet blt) { 
                    return blt.id == bullet.id;
                });
                
                removeBulletPacket(bullet.id, srv);
            }
        }

        bullet.pos.x += bullet.velocity.x;
        bullet.pos.y += bullet.velocity.y;
        bullet.lifeTime -= 1.f;
    }
}