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
            : orbVals[dif] * .8 * (float) percent / 100.f);
}

/*
Artifical version of the CurrencyRewardLayer. Never directly adds anything
*/
class $modify(ArtificalCRL, CurrencyRewardLayer) {
    struct Fields {
        bool visualOnly = false;
    };

    bool init(int orbs, int stars, int moons, int diamonds, CurrencySpriteType demonKey, 
                int keyCount, CurrencySpriteType shardType, int shardsCount, CCPoint position, 
                CurrencyRewardType rewardType, float yoffset, float time) {
        if (!CurrencyRewardLayer::init(orbs, stars, moons, diamonds, demonKey, keyCount, shardType, shardsCount, position, rewardType, yoffset, time)) return false;
        log::debug("{}", position);
        return true;
    }

    #ifndef GEODE_IS_WINDOWS
    void incrementCount(int count) {
        if (m_fields->visualOnly) count = 0;
        CurrencyRewardLayer::incrementCount(count);
    }
    # endif

    void setVisualOnly(bool realOrFake) {
        m_fields->visualOnly = realOrFake;
    }

    static ArtificalCRL* createArtifical(int orbs, int diamonds, CCPoint position, CurrencyRewardType rewardType, float time) {
        ArtificalCRL* temp = reinterpret_cast<ArtificalCRL*>(CurrencyRewardLayer::create(orbs, 0, 0, diamonds, CurrencySpriteType::Icon, 0, CurrencySpriteType::Icon, 0, position, rewardType, 0, time));
        if (temp != nullptr) temp->setVisualOnly(true);
        log::debug("a");
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
        int prevBest = m_fields->currentBest;
        log::debug("prevBest: {}", prevBest);

        PlayLayer::showNewBest(p0, p1, p2, p3, p4, p5);
        GameStatsManager* gsm = GameStatsManager::sharedState();
        const int levelOrbs = gsm->getAwardedCurrencyForLevel(m_level);
        log::debug("orbs: {}", levelOrbs);

        int dif = m_level->m_stars.value();
        if (dif <= 0) return;

        int tempO = orbCalc(getCurrentPercentInt(),dif);
        if (tempO - levelOrbs < 0) {
            int orbInput = orbCalc(prevBest, dif) - tempO;
            CCPoint pos = {210.79822, 194.24483};
            ArtificalCRL* ACRL = ArtificalCRL::createArtifical(orbInput, 0, pos, CurrencyRewardType::Default, .9);
            ACRL->setID("Artifical_CurrencyRewardLayer"_spr);
            this->addChild(ACRL);
        }
        m_fields->currentBest = getCurrentPercentInt();
    }
};