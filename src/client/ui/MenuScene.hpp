#include <Scene.hpp>
#include <elements/Button.hpp>
#include <elements/Textbox.hpp>
#include <elements/Image.hpp>

class MenuScene : public Scene {
public:
    MenuScene();

    void textEdited(Gui::Textbox* textbox, std::string str);
    void buttonClicked(Gui::Button* button);
protected:
    Gui::Textbox m_username;
    Gui::Textbox m_ip;
    Gui::Button  m_play;
    Gui::Button  m_editor;
    Gui::Button  m_arrowRight;
    Gui::Button  m_arrowLeft;
    Gui::Image   m_player;
    Gui::Image   m_hat;
private:
    void setHatBounds(Hat hat);
};