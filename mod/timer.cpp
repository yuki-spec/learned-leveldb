//
// Created by daiyi on 2020/02/02.
//

#include "timer.h"
#include "util.h"
#include <cassert>


namespace adgMod {

    Timer::Timer() : timestamp_accumulated(0), started(false) {
        clock_gettime(CLOCK_MONOTONIC, &time_initial);
    }

    void Timer::Start() {
        assert(!started);
        clock_gettime(CLOCK_MONOTONIC, &time_started);
        started = true;
    }

    void Timer::Pause(bool record) {
        assert(started);
        struct timespec time_ended;
        clock_gettime(CLOCK_MONOTONIC, &time_ended);
        uint64_t time_elapse = get_time_difference(time_started, time_ended);
        timestamp_accumulated += time_elapse;
        if (record)
            time_series.push_back(std::make_pair(get_time_difference(time_initial, time_started), get_time_difference(time_initial, time_ended)));
        started = false;
    }

    void Timer::Reset(bool record) {
        timestamp_accumulated = 0;
        started = false;
        if (record) {
            clock_gettime(CLOCK_MONOTONIC, &time_initial);
            time_series.clear();
        }
    }

    uint64_t Timer::Time() {
        assert(!started);
        return timestamp_accumulated;
    }

    void Timer::ReportTimeSeries() {
        for (auto& pair: time_series) printf("(%lu %lu)", pair.first, pair.second);
    }

}