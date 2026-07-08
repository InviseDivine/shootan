#pragma once
#include <Scene.hpp>

class ErrorScene : public Scene {
private:

public:
    ErrorScene() { m_bgColor = WHITE; };
    virtual ~ErrorScene() = default;
    virtual void render();
};