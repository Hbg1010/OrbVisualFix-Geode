#include <Geode/Geode.hpp>
#include <Geode/modify/CurrencyRewardLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>
#include <algorithm>
#include <string>

using namespace geode::prelude;

// calculate the amount of orbs to display
int orbCalc(int percent, int dif, bool isMainLevel) {
    if (isMainLevel && dif < 14) {
        float scalar = 25.f + 25*dif;
        return percent == 100 
            ? scalar 
            : scalar * .8f * (float) percent / 100.f;
    }

    const float orbVals[10] = {0.f, 50.f, 75.f, 125.f, 175.f, 225.f, 275.f, 350.f, 425.f, 500.f};
    dif = std::clamp(dif, 1, 10) - 1;

    return (int) (percent == 100
            ? orbVals[dif] 
            : orbVals[dif] * .8f * (float) percent / 100.f);
}

// calculate diamonds player would have earned
int diamondsCalc(int percent, int dif) {
    if (dif <= 1) return 0;
    dif = std::clamp(dif, 2, 10);
    return (2 + dif) * percent / 100;
}

// i just wanted to shorten this constructor lol
CurrencyRewardLayer* createArtificalCRL(int orbs, int diamonds, CCPoint position, CurrencyRewardType rewardType, float time) {
    return CurrencyRewardLayer::create(orbs, 0, 0, diamonds, CurrencySpriteType::Icon, 0, CurrencySpriteType::Icon, 0, position, rewardType, 0, time);
}

class $modify(VisualFixPL, PlayLayer) {
    struct Fields {
        int currentBest = 0;
        bool obtainedAllOrbs = false;
        bool isSpecial = false;
        bool isMainLevel = false;
    };

    bool init(GJGameLevel* p0, bool p1, bool p2) {
        if (!PlayLayer::init(p0,p1,p2)) return false;
        if (!m_isPlatformer) m_fields->currentBest = p0->getNormalPercent();

        // early returned if unrated
        int dif = m_level->m_stars.value();
        if (dif <= 0) return true;

        const int id = p0->m_levelID;
        // main level stuff bc rob made calcs weird for this
        m_fields->isMainLevel = ((id >= 1 && id <= 22) || id == 3001 || (id >= 5001 && id <= 5004));

        GameStatsManager* gsm = GameStatsManager::sharedState();
        const int levelOrbs = gsm->getAwardedCurrencyForLevel(p0);

        m_fields->isSpecial = (m_level->m_dailyID != 0 || m_level->m_gauntletLevel); 
        m_fields->obtainedAllOrbs = (levelOrbs == orbCalc(100, dif, m_fields->isMainLevel));

        return true;
    }

    void showNewBest(bool showRewards, int orbs, int diamonds, bool p3, bool p4, bool p5) {
        int dif = m_level->m_stars.value();
        bool isFake = false;
        if (dif > 0 && Mod::get()->getSettingValue<bool>("enable")) {
            GameStatsManager* gsm = GameStatsManager::sharedState();
            const int levelOrbs = gsm->getAwardedCurrencyForLevel(m_level);

            int tempO = orbCalc(getCurrentPercentInt(),dif, m_fields->isMainLevel);
            if (tempO - levelOrbs < 0) {
                if (Mod::get()->getSettingValue<bool>("hideNewRewardTxt")) isFake = true;
                int prevBest = m_fields->currentBest;
                // showRewards = true;
                orbs = tempO - orbCalc(prevBest, dif, m_fields->isMainLevel);

                // calculate collected diamonds restricting to just levels that have diamonds
                if (Mod::get()->getSettingValue<bool>("enableDiamonds") && m_fields->isSpecial) {
                    int collectedDia = gsm->getAwardedDiamondsForLevel(m_level);
                    int diaNow = diamondsCalc(getCurrentPercentInt(), dif);
                    if (diaNow - collectedDia < 0) {
                        diamonds = diamondsCalc(getCurrentPercentInt(), dif) - diamondsCalc(prevBest, dif);
                    }
                }
            }
        }

        PlayLayer::showNewBest(showRewards, orbs, diamonds, p3, p4, p5);
        m_fields->currentBest = m_level->getNormalPercent();

        // hides the "new reward text" or something. not totally clear...
        // isFake can only be true when the setting is true
        if (isFake) {
            if (CCNode* bestNode = getNewBestNode()) {
                auto children = bestNode->getChildren();
                for (int i = 0; i < children->count()-1; i++) {
                    CCNode* child = static_cast<CCNode*>(children->objectAtIndex(i));
                    if (auto txt = typeinfo_cast<CCLabelBMFont*>(child)) {
                        const char* labelTxt = txt->getString();
                        if (labelTxt != nullptr && sizeof(labelTxt) > 1) {
                            if (labelTxt[0] == '+' && strnlen(labelTxt, 4) <= 4) txt->setVisible(false);
                        }
                }
            }
            if (CCNode* orbImg = getChildBySpriteFrameName(bestNode, "currencyOrbIcon_001.png")) orbImg->setVisible(false);
            if (CCNode* diaImg = getChildBySpriteFrameName(bestNode, "GJ_bigDiamond_001.png")) diaImg->setVisible(false);
            }
        }
    }

    /*
    Extra methods
    */

    // reused from Ery / Hide New Best mod, ty Ery :)
    CCNode* getNewBestNode() {
        auto children = this->getChildren();
        for (int i = this->getChildrenCount() - 1; i >= 0; i--) {
            auto child = static_cast<CCNode*>(children->objectAtIndex(i));
            if (!child || child == this->m_uiLayer) continue; // skip UILayer
            if (child->getZOrder() != 100) continue;
            if (child->getChildrenCount() < 2) continue;
            child->setUserObject("new-best-node"_spr, CCBool::create(true)); // set the user object to identify the node
            return child;
        }
        return nullptr;
    }

    bool hasAllOrbs() {
        return m_fields->obtainedAllOrbs;
    }

    int currentBest() {
        return m_fields->currentBest;
    }

    // note: only checks for 
    bool isSpecial() {
        return m_fields->isSpecial;
    }

    void setCompleted() {
        m_fields->currentBest = 100;
    }

    bool mainLevel() {
        return m_fields->isMainLevel;
    }
};

/*
Artifical version of the CurrencyRewardLayer. Never directly adds anything
*/
class $modify(ArtificalCRL, CurrencyRewardLayer) {
    bool init(int orbs, int stars, int moons, int diamonds, CurrencySpriteType demonKey, 
                int keyCount, CurrencySpriteType shardType, int shardsCount, CCPoint position, 
                CurrencyRewardType rewardType, float yoffset, float time) {
        // checks if the level is rated and if the level has been completed before. Then alters visual case.
        if ((stars > 0 || moons > 0) && Mod::get()->getSettingValue<bool>("enable")) {
            if (VisualFixPL* VPL = geode::cast::modify_cast<VisualFixPL*>(PlayLayer::get())) {
                if (VPL->hasAllOrbs()) {
                    int dif = std::max(stars, moons);
                    int prevBest = VPL->currentBest();
                    orbs += orbCalc(100, dif, VPL->mainLevel()) - orbCalc(prevBest, dif, VPL->mainLevel());
                    if (VPL->isSpecial()) {
                        diamonds += diamondsCalc(100, dif) - diamondsCalc(prevBest, dif);
                    }
                    VPL->setCompleted();
                }
            }
        }

        // runs default function with possible added options
        return CurrencyRewardLayer::init(orbs, stars, moons, diamonds, demonKey, keyCount, shardType, shardsCount, position, rewardType, yoffset, time);
    }
};

class $modify(ELL, EndLevelLayer) {
    void customSetup() {
        if ((m_stars > 0 || m_moons > 0) && Mod::get()->getSettingValue<bool>("enable")) {
            if (VisualFixPL* VPL = geode::cast::modify_cast<VisualFixPL*>(PlayLayer::get())) {
                if (VPL->hasAllOrbs()) {
                    int dif = std::max(m_stars, m_moons);
                    int prevBest = VPL->currentBest();
                    m_orbs = orbCalc(100, dif, VPL->mainLevel()) - orbCalc(prevBest, dif, VPL->mainLevel());
                    if (VPL->isSpecial()) {
                        m_diamonds = diamondsCalc(100, dif) - diamondsCalc(prevBest, dif);
                    }
                    VPL->setCompleted();
                }
            }
        }
        EndLevelLayer::customSetup();
    }
};