#pragma once

#include "Player.h"
#include "SaveGame.h"
#include "UI.h"

#include <memory>
#include <string>
#include <vector>

class Shop;
class Monster;

// 游戏主控制器：负责标题、城门登记、身份牌存档、营地、商店、背包与遗迹流程。
class Game {
public:
    Game();
    void run();

private:
    void showTitleScreen() const;
    bool startFromGate();
    void renderRoad() const;
    bool showGatekeeper();
    void renderGate(const SaveSummary& summary, const std::string& message) const;
    bool registerNewIdentity(std::string& message);
    std::string readIdentityText(const std::string& prompt, int maxWidth) const;
    bool chooseIdentityGender(std::string& gender) const;
    bool chooseIdentitySpecies(std::string& species) const;
    void showCityEntryNarration() const;

    void showEnterRuinsCutscene() const;
    void showEncounterCutscene(const Monster& monster) const;
    std::unique_ptr<Monster> createRandomMonster() const;
    void showReturnCutscene() const;
    void renderCamp(const std::string& message) const;
    void renderTravel(int progress, int playerColumn) const;
    void animateWalk(int fromColumn, int toColumn, int progress) const;
    void showStatusScreen() const;
    void showShop();
    void showInventoryScreen();
    void renderShop(const Shop& shop, const std::string& message) const;
    void renderInventory(const std::string& title,
                         const std::string& message,
                         bool selling) const;
    std::string buyFromShop(Shop& shop);
    std::string sellToShop();
    void showResult(const std::string& title,
                    const std::vector<std::string>& lines,
                    UI::Color color) const;
    std::string explore();
    void renderFarewell(const std::string& warning) const;

    Player player_;
};
