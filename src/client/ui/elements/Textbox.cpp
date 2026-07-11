#include "Textbox.hpp"
#include <raygui.h>
#include <Scene.hpp>

namespace Gui {

void Textbox::draw() {
    if (GuiTextBox(m_bounds, m_chars.data(), m_chars.size(), m_focused)) m_focused = true;

    if (GUI_BUTTON_DOWN && !CheckCollisionPointRec(GetMousePosition(), m_bounds)) {
        if (m_focused && m_scene) m_scene->textEdited(this, std::string(m_chars.begin(), m_chars.end()));
        m_focused = false;
    }
}

};