//
// Created by daiyi on 2019/09/30.
//

#include <cassert>
#include "stats.h"
#include <cmath>
#include "plr.h"

using std::stoull;

namespace adgMod {

    Stats* Stats::singleton = nullptr;

    Stats::Stats() : timers(10, Timer{}), level_stats(9, 0) {}

    Stats* Stats::GetInstance() {
        if (!singleton) singleton = new Stats();
        return singleton;
    }

    void Stats::StartTimer(uint32_t id) {
        Timer& timer = timers[id];
        timer.Start();
    }

    void Stats::PauseTimer(uint32_t id, bool record) {
        Timer& timer = timers[id];
        timer.Pause(record);
    }

    void Stats::ResetTimer(uint32_t id) {
        Timer& timer = timers[id];
        timer.Reset();
    }

    uint64_t Stats::ReportTime(uint32_t id) {
        Timer& timer = timers[id];
        return timer.Time();
    }

    void Stats::ReportTime() {
        for (int i = 0; i < timers.size(); ++i) {
            printf("Timer %u: %lu\n", i, timers[i].Time());
        }
    }



    void Stats::RecordLevel(int level) {
        level_stats[level]++;
    }

    void Stats::ReportLevelStats() {
        for (int i = 0; i < level_stats.size(); ++i) {
            printf("Level %d: %u\n", i, level_stats[i]);
        }
    }

    void Stats::ResetAll() {
        for (Timer& t: timers) t.Reset();
        for (uint32_t& num: level_stats) num = 0;
    }

    Stats::~Stats() {
        ReportTime();
    }

}

































