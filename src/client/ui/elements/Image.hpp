#pragma once
#include "Element.hpp"
#include <Types.hpp>

namespace Gui {
    class Image : public Element {
    public:
        Image(const Sprite id = PLAYER_SPRITE, float rotation = 0, Vector2 origin = { 0 }, Color color = WHITE, bool flip = false) 
        : Image({ 0 }, id, rotation, origin, color, flip) {}
        Image(Rectangle bounds = { 0 }, const Sprite id = PLAYER_SPRITE, float rotation = 0, 
            Vector2 origin = { 0 }, Color color = WHITE, bool flip = false) 
        : Element(bounds), m_spriteId(id), m_rotation(rotation), m_origin(origin), m_color(color), m_flip(flip) {}
        
        void setSpriteId(Sprite id) { m_spriteId = id; }
        void draw();

    protected:
        Sprite m_spriteId;
        
        float m_rotation;
        Vector2 m_origin;
        
        Color m_color;
        
        bool m_flip;
    };
};