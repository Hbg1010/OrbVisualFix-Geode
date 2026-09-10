#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>
#include <algorithm>
#include <string_view>

using namespace geode::prelude;

// calculate the amount of orbs to display. I could replace with built in functions but these do their jobs
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

// gets the completed orb count
int baseOrbCount(int dif, bool isMainLevel) {
    return orbCalc(100, dif, isMainLevel);
}

// calculate diamonds player would have earned
int diamondsCalc(int percent, int dif) {
    if (dif <= 1) return 0;
    dif = std::clamp(dif, 2, 10);
    return (2 + dif) * percent / 100;
}

int baseDiaCount(int dif) {
    return diamondsCalc(100, dif);
}

class $modify(VisualFixPL, PlayLayer) {
    struct Fields {
        int currentBest = 0;
        bool obtainedAllOrbs = false;
    };

    bool init(GJGameLevel* p0, bool p1, bool p2) {
        if (!PlayLayer::init(p0,p1,p2)) return false;
        if (!m_isPlatformer) m_fields->currentBest = p0->getNormalPercent();

        // early returned if unrated
        int dif = m_level->m_stars.value();
        if (dif <= 0) return true;
        
        GameStatsManager* gsm = GameStatsManager::sharedState();
        const int levelOrbs = gsm->getAwardedCurrencyForLevel(p0);

        m_fields->obtainedAllOrbs = (levelOrbs == baseOrbCount(dif, mainLevel()));

        return true;
    }

    void showNewBest(bool showRewards, int orbs, int diamonds, bool p3, bool p4, bool p5) {
        int dif = m_level->m_stars.value();
        bool isFake = false;

        if (dif > 0 && Mod::get()->getSettingValue<bool>("enable")) {
            GameStatsManager* gsm = GameStatsManager::sharedState();
            const int levelOrbs = gsm->getAwardedCurrencyForLevel(m_level);

            // clamp because some levels may hit 100% bc of incorrect timing
            int tempO = orbCalc(std::clamp(getCurrentPercentInt(),0,99), dif, mainLevel());
            if (tempO - levelOrbs < 0) {
                if (Mod::get()->getSettingValue<bool>("hideNewRewardTxt")) isFake = true;
                int prevBest = m_fields->currentBest;

                orbs = tempO - orbCalc(prevBest, dif, mainLevel());

                // calculate collected diamonds restricting to just levels that have diamonds
                if (Mod::get()->getSettingValue<bool>("enableDiamonds") && isSpecial()) {
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
                        std::string_view labelTxt(txt->getString());
                        if (utils::string::startsWith(labelTxt, "+") && labelTxt.size() <= 4) txt->setVisible(false);
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

    bool isSpecial() {
        return (m_level->m_dailyID != 0 || m_level->m_gauntletLevel);
    }

    void setCompleted() {
        m_fields->currentBest = 100;
    }

    bool mainLevel() {
        return m_level->m_levelType == GJLevelType::Main;
    }
};

class $modify(ELL, EndLevelLayer) {
    void customSetup() {
        int dif = m_playLayer->m_level->m_stars.value();

        // checks to see if the orb visuals should be used
        if (dif > 0 && !m_playLayer->m_isPracticeMode 
            && !m_playLayer->m_isTestMode && Mod::get()->getSettingValue<bool>("enable")) {
            if (VisualFixPL* VPL = geode::cast::modify_cast<VisualFixPL*>(m_playLayer)) {
                if (VPL->hasAllOrbs()) {
                    int prevBest = VPL->currentBest();
                    m_playLayer->m_orbs = baseOrbCount(dif, VPL->mainLevel()) - orbCalc(prevBest, dif, VPL->mainLevel());
                    if (VPL->isSpecial()) {
                        m_playLayer->m_diamonds = baseDiaCount(dif) - diamondsCalc(prevBest, dif);
                    }
                    VPL->setCompleted();
                }
            } 
        }
        EndLevelLayer::customSetup();
    }
};