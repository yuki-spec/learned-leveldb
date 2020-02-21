//
// Created by daiyi on 2020/02/02.
//

#include "timer.h"
#include "util.h"
#include <cassert>


namespace adgMod {

    Timer::Timer() : timestamp_accumulated(0), started(false) {}

    void Timer::Start() {
        assert(!started);
        time_started = __rdtsc();
        started = true;
    }

    void Timer::Pause(bool record) {
        assert(started);
        uint64_t time_elapse = __rdtsc() - time_started;
        timestamp_accumulated += time_elapse / reference_frequency;

        if (record) {
            Stats* instance = Stats::GetInstance();
            uint64_t start_absolute = time_started - instance->initial_time;
            uint64_t end_absolute = start_absolute + time_elapse;
            time_series.emplace_back(start_absolute / reference_frequency, end_absolute / reference_frequency);
        }

        started = false;
    }

    void Timer::Reset(bool record) {
        timestamp_accumulated = 0;
        started = false;
        if (record) {
            time_series.clear();
        }
    }

    uint64_t Timer::Time() {
        //assert(!started);
        return timestamp_accumulated;
    }

    void Timer::ReportTimeSeries() {
        for (auto& pair: time_series) printf("(%lu %lu)", pair.first, pair.second);
    }

}