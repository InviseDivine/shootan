#include "raylib.h"
#include <raygui.h>
#include <ErrorScene.hpp>
#include <Multiplayer.hpp>
#include <Game.hpp>

void ErrorScene::render() {
    auto reason = Multiplayer::get().getStringReason();

    auto fontSize = 35;

    auto text = TextFormat("Sadly you was disconnected :( Reason: %s", reason.c_str());

    auto textWidth = MeasureText(text, fontSize);
    DrawText(text, (GetScreenWidth() - textWidth) / 2, (GetScreenHeight() - fontSize) / 2, fontSize, BLACK);

    Rectangle reconnect = { 0 };
    reconnect.width = 200.f;
    reconnect.height = 50.f;
    reconnect.x = (GetScreenWidth() - reconnect.width) / 2;
    reconnect.y = (GetScreenHeight() - reconnect.height) / 2 + fontSize + 10.f;

    if (GuiButton(reconnect, "Reconnect")) {
        auto& game = Game::get();

        game.startMpThread();
        game.clearScene();
    }
}