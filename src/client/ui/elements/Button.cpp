#include "Button.hpp"
#include <raygui.h>
#include <Scene.hpp>

namespace Gui {

void Button::draw() {
    if (GuiButton(m_bounds, m_label.c_str()) && m_scene) {
        m_scene->buttonClicked(this);
    }
}

};