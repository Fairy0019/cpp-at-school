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
    void showPrologueScreen() const;
    bool showStoryBeat();
    bool startFromGate();
    void renderRoad() const;
    void showRoadLeaveScreen() const;
    bool showGatekeeper();
    std::vector<std::string> gateDialogue(const SaveSummary& summary) const;
    void renderGate(const SaveSummary& summary, const std::string& message,
                    const std::string& dialogueLine, bool showMenu) const;
    bool registerNewIdentity(std::string& message);
    std::string readIdentityText(const std::string& prompt, int maxWidth) const;
    bool chooseIdentityGender(std::string& gender) const;
    bool chooseIdentitySpecies(std::string& species) const;
    void showCityEntryNarration() const;
    void showReawakening() const;

    void showEnterRuinsCutscene() const;
    void showAreaIntro(int stage) const;
    void showEncounterCutscene(const Monster& monster) const;
    std::unique_ptr<Monster> createRandomMonster(int floor) const;
    std::unique_ptr<Monster> createGuardian() const;
    std::unique_ptr<Monster> createMimic() const;
    std::string showRewardRoom(bool& defeated);
    void grantChestLoot(std::vector<std::string>& lines);
    void showReturnCutscene() const;
    void showCoreSealedScene() const;
    bool enterCore();
    void showEnding(int choice) const;
    void renderCamp(const std::string& message) const;
    void showCampTalk();
    void renderCorridor(int floor, int depth, int areaStage, int playerColumn = 48) const;
    void animateCorridorStep(int floor, int depth, int areaStage, bool moveRight) const;
    void handleCorridorTrap(int floor);
    void handleCorridorCaveIn(int floor, int& nextFloor);
    void handleCorridorCorpse();
    bool handleCorridorDoor(int& floor, std::string& message);
    std::string handleDefeat(const std::string& enemyName);
    void triggerTravelDisaster();
    void showStatusScreen() const;
    void showShop();
    void showTrader();
    void showShopScreen(bool full);
    void refreshCampShop();
    void showInventoryScreen();
    void renderShop(const Shop& shop, const std::string& message, bool full) const;
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
    int storyStage_;
    int storyEnding_;
    bool shopAvailable_;
    bool resumedFromSave_;
};
