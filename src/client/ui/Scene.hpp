#pragma once
#include <raylib.h>
#include <raygui.h>

class Scene {
protected:
    Color m_bgColor = BLACK;
public:
    Scene() {};
    virtual ~Scene() = default;
    Color getColor() {
        return m_bgColor;
    }

    void setColor(Color color) {
        m_bgColor = color;
    }

    virtual void update() {}
    virtual void render() {}
};