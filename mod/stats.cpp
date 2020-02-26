//
// Created by daiyi on 2019/09/30.
//

#include <cassert>
#include "stats.h"
#include <cmath>
#include <iostream>
#include "plr.h"
#include "util.h"

using std::stoull;

namespace adgMod {

    Stats* Stats::singleton = nullptr;

    Stats::Stats() : timers(12, Timer{}), level_stats(9, 0), counters(10), initial_time(__rdtsc()) {}

    Stats* Stats::GetInstance() {
        if (!singleton) singleton = new Stats();
        return singleton;
    }

    void Stats::StartTimer(uint32_t id) {
        Timer& timer = timers[id];
        timer.Start();
        //if (id > 6) std::cout << "Timer " << id << " started." << std::endl;
    }

    void Stats::PauseTimer(uint32_t id, bool record) {
        Timer& timer = timers[id];
        timer.Pause(record);
        //if (id > 6) std::cout << "Timer " << id << " paused." << std::endl;
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

    void Stats::ReportTimeSeries(uint32_t id) {
        printf("TimeSeries %u: ", id);
        timers[id].ReportTimeSeries();
        printf("\n");
    }



    void Stats::RecordLevel(int level) {
        level_stats[level]++;
    }

    void Stats::ReportLevelStats() {
        for (int i = 0; i < level_stats.size(); ++i) {
            printf("Level %d: %u\n", i, level_stats[i]);
        }
    }


    void Stats::IncrementCounter(int id) {
        counters[id].Increment();
    }

    int Stats::ReportCounter(int id) {
        return counters[id].Report();
    }

    void Stats::ResetCounter(int id) {
        counters[id].Reset();
    }

    void Stats::ReportEventWithTime(const string &event) {
        uint64_t time_absolute = __rdtsc() - initial_time;
        printf("%s %lu\n", event.c_str(), (uint64_t) (time_absolute / reference_frequency));
    }




    void Stats::ResetAll(bool record) {
        for (Timer& t: timers) t.Reset(record);
        for (uint32_t& num: level_stats) num = 0;
        for (Counter& c: counters) c.Reset();
        initial_time = __rdtsc();
    }

    Stats::~Stats() {
        ReportTime();
    }

}

































