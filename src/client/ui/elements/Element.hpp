#pragma once
#include <raylib.h>

class Scene;

namespace Gui {
    class Element {
    public:
        Element(Rectangle bounds = { 0 }) : m_bounds(bounds) {}

        void setBounds(Rectangle bounds) { m_bounds = bounds; }
        Rectangle getBounds() { return m_bounds; }

        void setFocused(bool focused) { m_focused = focused; }
        bool isFocused() { return m_focused; }

        virtual void draw() = 0;

        void setScene(Scene* scene) { m_scene = scene; }
    protected:
        Rectangle m_bounds;
        Scene* m_scene;
        bool m_focused;
    };
};