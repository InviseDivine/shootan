#include <MenuScene.hpp>
#include <raylib.h>
#include <Game.hpp>

MenuScene::MenuScene() : m_play("Play online"), m_editor("Editor mode") {
    setColor(WHITE);

    auto& game = Game::get();

    Rectangle bounds = {(GetScreenWidth() - 200.f) / 2.f, (GetScreenHeight() - 22.f) / 2.f, 200.f, 22.f};
    m_username.setBounds(bounds);
    m_username.setString(game.getPlayer().nickname);
    registerElement(&m_username);

    bounds.y += bounds.height + 10.f;
    m_play.setBounds(bounds);
    registerElement(&m_play);

    bounds.y += bounds.height + 10.f;
    m_editor.setBounds(bounds);
    registerElement(&m_editor);
}

void MenuScene::textEdited(Gui::Textbox* textbox, std::string str) {
    auto& game = Game::get();

    if (textbox == &m_username) {
        game.getPlayer().nickname = str;
        return;
    }
}

void MenuScene::buttonClicked(Gui::Button* button) {
    auto& game = Game::get();
    
    if (button == &m_play) {
        game.exitEditor();
        game.startMpThread();
        game.clearScene();
        return;
    }

    if (button == &m_editor) {
        game.getLevel().read();
        game.enterEditor();
        game.clearScene();
        return;
    }
}