#include "Game.h"
#include "UI.h"

int main() {
    UI::setupConsole();

    Game game;
    game.run();

    return 0;
}
