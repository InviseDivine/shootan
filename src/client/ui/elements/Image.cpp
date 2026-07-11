#include "Image.hpp"
#include <raygui.h>
#include <Scene.hpp>
#include <ResourceManager.hpp>

namespace Gui {

void Image::draw() {
    auto& rm = ResourceManager::get();
    
    if (m_spriteId < SPRITES_COUNT) {
        rm.drawSpriteFromSheet(m_spriteId, m_bounds, m_origin, m_rotation, m_color, m_flip);
    }
}

};