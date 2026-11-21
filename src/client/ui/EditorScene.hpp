#pragma once

#include <Scene.hpp>

class EditorScene : public Scene {
private:

public:
    EditorScene() { m_bgColor = WHITE; };
    virtual ~EditorScene() = default;
    virtual void render();
};