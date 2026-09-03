#include <Geode/Geode.hpp>
#include <Geode/modify/CurrencyRewardLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <algorithm>

using namespace geode::prelude;

// calculate the amount of orbs to display
int orbCalc(int percent, int dif) {
    // TODO: Impl fixes for main levels that have custom values (clamp SHOULD make it safe but still)
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
    };

    bool init(GJGameLevel* p0, bool p1, bool p2) {
        if (!PlayLayer::init(p0,p1,p2)) return false;
        if (!m_isPlatformer) m_fields->currentBest = p0->getNormalPercent();

        // early returned if unrated
        int dif = m_level->m_stars.value();
        if (dif <= 0) return true;

        GameStatsManager* gsm = GameStatsManager::sharedState();
        const int levelOrbs = gsm->getAwardedCurrencyForLevel(p0);

        // for end screen
        // checks for if it can give diamonds
        if (m_level->m_dailyID != 0 || m_level->m_gauntletLevel) {
                m_fields->isSpecial = true;
        }
        if (levelOrbs == orbCalc(100, dif)) {
            m_fields->obtainedAllOrbs = true;
        } else {
            m_fields->obtainedAllOrbs = false;
             m_fields->isSpecial = false;

        }

        return true;
    }

    void showNewBest(bool p0, int p1, int p2, bool p3, bool p4, bool p5) {
        int dif = m_level->m_stars.value();

        if (dif > 0) {
            GameStatsManager* gsm = GameStatsManager::sharedState();
            const int levelOrbs = gsm->getAwardedCurrencyForLevel(m_level);

            int tempO = orbCalc(getCurrentPercentInt(),dif);
            if (tempO - levelOrbs < 0) {
                int prevBest = m_fields->currentBest;
                p0 = true;
                p1 = tempO - orbCalc(prevBest, dif);

                // calculate collected diamonds restricting to just levels that have diamonds
                int diaInput = 0;
                if (m_level->m_dailyID != 0 || m_level->m_gauntletLevel) {
                    int collectedDia = gsm->getAwardedDiamondsForLevel(m_level);
                    int diaNow = diamondsCalc(getCurrentPercentInt(), dif);
                    if (diaNow - collectedDia < 0) {
                        p2 = diamondsCalc(getCurrentPercentInt(), dif) - diamondsCalc(prevBest, dif);
                    }
                }
            }
        } 
        
        PlayLayer::showNewBest(p0, p1, p2, p3, p4, p5);
        
        
        

            // // get positions TODO
            // CCSize screenSize = CCDirector::sharedDirector()->getWinSize();
            // CCPoint pos = {screenSize.width/2.f, screenSize.height/2.f};

            // CurrencyRewardLayer* ACRL = createArtificalCRL(orbInput, diaInput, pos, CurrencyRewardType::Default, .9);
            // ACRL->setZOrder(99);
            // ACRL->setID("Artifical_CurrencyRewardLayer"_spr);
            // this->addChild(ACRL);

            // // lord help the decomp output is worse than the song dance monkey its actually so bad

            // // _ccColor4B color = {0,0,0,0};
            // // CCLayerColor* CCLC = CCLayerColor::create(color);
            // // this->addChild(CCLC, 99);
            // // CCFiniteTimeAction* FTA = (CCFiniteTimeAction*) CCFadeTo::create(0.3, 100);
            // // CCDelayTime* CCDT = CCDelayTime::create(1.6f);
            // // CCFadeTo* CCFT = CCFadeTo::create(.4f, 0);
            // // CCCallFunc* uVar19 = CCCallFunc::create(CCLC, callfunc_selector(CCNode::removeMeAndCleanup));   
            // // CCAction* pCVar21 = CCSequence::create(FTA, CCDT, CCFT, uVar19);

            // // CCLC->runAction(pCVar21);

            // // play sounds 
            // FMODAudioEngine* fmod = FMODAudioEngine::sharedEngine();
            // fmod->playEffect(diaInput > 0 ? "gold02.ogg" : "magicExplosion.ogg");

            // // reset player's best
            // m_fields->currentBest = getCurrentPercentInt();
    }

    /*
    Extra methods (getters + setters)
    */

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
};

/*
Artifical version of the CurrencyRewardLayer. Never directly adds anything
*/
class $modify(ArtificalCRL, CurrencyRewardLayer) {
    bool init(int orbs, int stars, int moons, int diamonds, CurrencySpriteType demonKey, 
                int keyCount, CurrencySpriteType shardType, int shardsCount, CCPoint position, 
                CurrencyRewardType rewardType, float yoffset, float time) {
        // checks if the level is rated and if the level has been completed before. Then alters visual case.
        if (stars > 0 || moons > 0) {
            if (VisualFixPL* VPL = reinterpret_cast<VisualFixPL*>(PlayLayer::get())) {
                if (VPL->hasAllOrbs()) {
                    int dif = std::max(stars, moons);
                    int prevBest = VPL->currentBest();
                    orbs += orbCalc(100, dif) - orbCalc(prevBest, dif);
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