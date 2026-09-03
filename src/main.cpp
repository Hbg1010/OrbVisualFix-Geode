#include <Geode/Geode.hpp>
#include <Geode/modify/CurrencyRewardLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <algorithm>

using namespace geode::prelude;

// calculate the amount of orbs to display
int orbCalc(int percent, int dif) {
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

/*
Artifical version of the CurrencyRewardLayer. Never directly adds anything
*/
class $modify(ArtificalCRL, CurrencyRewardLayer) {
    // struct Fields {
    //     bool visualOnly = false;
    // };

    bool init(int orbs, int stars, int moons, int diamonds, CurrencySpriteType demonKey, 
                int keyCount, CurrencySpriteType shardType, int shardsCount, CCPoint position, 
                CurrencyRewardType rewardType, float yoffset, float time) {
        if (!CurrencyRewardLayer::init(orbs, stars, moons, diamonds, demonKey, keyCount, shardType, shardsCount, position, rewardType, yoffset, time)) return false;
        // geode::log::debug("{}", position);
        return true;
    }

    // i just wanted to shorten this constructor lol
    static ArtificalCRL* createArtifical(int orbs, int diamonds, CCPoint position, CurrencyRewardType rewardType, float time) {
        ArtificalCRL* temp = reinterpret_cast<ArtificalCRL*>(CurrencyRewardLayer::create(orbs, 0, 0, diamonds, CurrencySpriteType::Icon, 0, CurrencySpriteType::Icon, 0, position, rewardType, 0, time));
        return temp;
    }
};

class $modify(bestFinder, PlayLayer) {
    struct Fields {
        int currentBest = 0;
    };

    bool init(GJGameLevel* p0, bool p1, bool p2) {
        if (!PlayLayer::init(p0,p1,p2)) return false;
        if (!m_isPlatformer) m_fields->currentBest = p0->getNormalPercent();
        return true;
    }

    void showNewBest(bool p0, int p1, int p2, bool p3, bool p4, bool p5) {
        PlayLayer::showNewBest(p0, p1, p2, p3, p4, p5);
        
        int dif = m_level->m_stars.value();
        if (dif <= 0) return;
        GameStatsManager* gsm = GameStatsManager::sharedState();
        const int levelOrbs = gsm->getAwardedCurrencyForLevel(m_level);

        int tempO = orbCalc(getCurrentPercentInt(),dif);
        if (tempO - levelOrbs < 0) {
            int prevBest = m_fields->currentBest;
            int orbInput = tempO - orbCalc(prevBest, dif);

            // calculate collected diamonds restricting to just levels that have diamonds
            int diaInput = 0;
            if ((m_level->m_dailyID != 0 || m_level->m_gauntletLevel)) {
                int collectedDia = gsm->getAwardedDiamondsForLevel(m_level);
                int diaNow = diamondsCalc(getCurrentPercentInt(), dif);
                if (diaNow - collectedDia < 0) {
                    diaInput = diamondsCalc(getCurrentPercentInt(), dif) - diamondsCalc(prevBest, dif);
                }
            }

            // get positions TODO
            CCSize screenSize = CCDirector::sharedDirector()->getWinSize();
            CCPoint pos = {screenSize.width/2.f, screenSize.height/2.f};

            ArtificalCRL* ACRL = ArtificalCRL::createArtifical(orbInput, diaInput, pos, CurrencyRewardType::Default, .9);
            ACRL->setZOrder(99);
            ACRL->setID("Artifical_CurrencyRewardLayer"_spr);
            this->addChild(ACRL);

            // play sounds 
            FMODAudioEngine* fmod = FMODAudioEngine::sharedEngine();
            fmod->playEffect(diaInput > 0 ? "gold02.ogg" : "magicExplosion.ogg");

            // reset player's best
            m_fields->currentBest = getCurrentPercentInt();
        }
    }
};