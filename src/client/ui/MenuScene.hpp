#include <Scene.hpp>
#include <elements/Button.hpp>
#include <elements/Textbox.hpp>

class MenuScene : public Scene {
public:
    MenuScene();

    void textEdited(Gui::Textbox* textbox, std::string str);
    void buttonClicked(Gui::Button* button);
protected:
    Gui::Textbox m_username;
    Gui::Button m_play;
    Gui::Button m_editor;
};