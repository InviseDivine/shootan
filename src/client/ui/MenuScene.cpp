#include <MenuScene.hpp>
#include <raylib.h>
#include <Game.hpp>
#include <ResourceManager.hpp>

void MenuScene::setHatBounds(Hat hat) {
    auto& game = Game::get();
    auto& rm = ResourceManager::get();

    auto& size = rm.getSpriteSize(rm.getHatSprite(game.getPlayer().hat));
    auto bounds = m_hat.getBounds();
    bounds.width = size.x * 6;
    bounds.height = size.y * 6;
    bounds.y = m_player.getBounds().y + rm.getHatPos(hat).y * 6;
    bounds.x = m_player.getBounds().x + rm.getHatPos(hat).x * 6;

    m_hat.setSpriteId(rm.getHatSprite(game.getPlayer().hat)); 
    m_hat.setBounds(bounds);
}

MenuScene::MenuScene() : m_play("Play online"), m_editor("Editor mode"), m_hat(SPRITES_COUNT), m_player(PLAYER_SPRITE), m_arrowLeft("<"), m_arrowRight(">") {
    setColor(RAYWHITE);

    auto& game = Game::get();

    Rectangle bounds = {(GetScreenWidth() - 200.f) / 2.f, (GetScreenHeight() - 22.f) / 2.f, 200.f, 22.f};
    m_username.setBounds(bounds);
    m_username.setString(game.getPlayer().nickname);
    registerElement(&m_username);

    // TODO: Label for hats choose
    Rectangle image = {(GetScreenWidth() - 48.f) / 2.f, (GetScreenHeight() - 48.f) / 2.f - 40.f - 48.f, 48.f, 48.f};
    m_player.setBounds(image);
    
    Rectangle arrow = {image.x + image.width + 24.f, image.y + 12.f, 24.f, 24.f};
    m_arrowRight.setBounds(arrow);
    registerElement(&m_arrowRight);

    arrow.x -= image.width + 24.f * 3;
    m_arrowLeft.setBounds(arrow);
    registerElement(&m_arrowLeft);

    image.y -= 48.f;
    registerElement(&m_player);

    m_hat.setBounds(image);
    registerElement(&m_hat);

    bounds.y += bounds.height + 10.f;
    m_ip.setBounds(bounds);
    m_ip.setString("sffempire.ru");
    registerElement(&m_ip);

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
        if (!m_ip.getString().empty()) {
            game.exitEditor();
            game.startMpThread(m_ip.getString());
            game.clearScene();   
        }
        return;
    }

    if (button == &m_editor) {
        game.getLevel().read();
        game.enterEditor();
        game.clearScene();
        return;
    }

    if (button == &m_arrowLeft) {
        auto& rm = ResourceManager::get();
        auto& hat = game.getPlayer().hat;

        if (hat - 1 < 0) {
            hat = (Hat)(HATS_COUNT - 1);
        } else {
            hat = (Hat)(hat - 1);
        }

        setHatBounds(hat);
        return;
    }

    if (button == &m_arrowRight) {
        auto& rm = ResourceManager::get();
        auto& hat = game.getPlayer().hat;

        hat = (Hat)(hat + 1);
        
        if (hat >= HATS_COUNT) {
            hat = NONE_HAT;
        }

        setHatBounds(hat);
        return;
    }  
}