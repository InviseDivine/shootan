#pragma once
#include <raylib.h>
#include <list>
#include <string>

namespace Gui {
    class Element;
    class Textbox;
    class Button;
    class Image;
};

class Scene {
protected:
    Color m_bgColor = BLACK;
    std::list<Gui::Element*> m_elements;

public:
    Scene() {};
    virtual ~Scene() = default;
    Color getColor() {
        return m_bgColor;
    }

    void setColor(Color color) {
        m_bgColor = color;
    }

    void registerElement(Gui::Element* element);

    virtual void update() {}
    virtual void render();

    virtual void textEdited(Gui::Textbox* textbox, std::string str) {}
    virtual void buttonClicked(Gui::Button* button) {}
};