#include <Geode/Geode.hpp>
#include <Geode/modify/CurrencyRewardLayer.hpp>

using namespace geode::prelude;

/*
Artifical version of the CurrencyRewardLayer. Never directly adds anything
*/
class $modify(ArtificalCRL, CurrencyRewardLayer) {
    struct Field {
        int m_originalCount;
    };

    void incrementCount(int count) {
        count = 0;
        CurrencyRewardLayer::incrementCount(count);
    }
};

class $modify(bestFinder, PlayLayer) {
    void showNewBest(bool p0, int p1, int p2, bool p3, bool p4, bool p5) {
        int prevBest = m_level->getNormalPercent();
        PlayLayer::showNewBest(p0, p1, p2, p3, p4, p5);

        // todo: impl orb formula based on input % 
        int tempO = orbCalc(getCurrentPercentInt());

        if (tempO - m_orbs < 0 && m_level->m_stars.value() > 0) {
            orbInput = orbCalc(prevBest) - tempO;
            CCSprite oSpr = CCSprite::createWithSpriteFrameName("currencyOrbIcon_001.png");
            //  CurrencyRewardLayer::create(0,0x3f666666,param_2,0,0,param_3,0,0,0,0,&local_10,0);
            ArtificalCRL = ArtificalCRL::create(orbInput, 0, 0, p2, 0, 0, 0, 0, oSpr, 0);
            ArtificalCRL->setID("Artifical_CLR"_spr);
            this->addChild(ArtificalCRL); // todo: see if it's ref count lowers, if not fix that
        }
    }

    int orbCalc(int percent, int dif) {
        // clamp
        if (percent < 0) = 0;
        else if (percent > 100) percent = 100;

        // todo: obtain formulas, add dif tree here
        int o = 0;
        return o;
    }
};