//
// Created by daiyi on 2019/09/30.
//

#include <cassert>
#include <x86intrin.h>
#include "stats.h"
#include "time.h"




namespace adgMod {
    bool MOD = false;

    class Timer {
        friend class Stats;

        void Start();
        void Pause();
        void Reset();
        uint64_t Time();

        struct timespec time_started;
        uint64_t timestamp_accumulated;
        bool started;

    public:
        Timer();
        ~Timer() = default;
    };

    Timer::Timer() : timestamp_accumulated(0), started(false) {}

    void Timer::Start() {
        assert(!started);
        clock_gettime(CLOCK_MONOTONIC, &time_started);
        started = true;
    }

    void Timer::Pause() {
        assert(started);
        struct timespec time_ended;
        clock_gettime(CLOCK_MONOTONIC, &time_ended);
        uint64_t time_elapse = (time_ended.tv_sec - time_started.tv_sec) * 1000000000 + time_ended.tv_nsec - time_started.tv_nsec;
        timestamp_accumulated += time_elapse;
        started = false;
    }

    void Timer::Reset() {
        timestamp_accumulated = 0;
        started = false;
    }

    uint64_t Timer::Time() {
        assert(!started);
        return timestamp_accumulated;
    }

    Stats* Stats::singleton = nullptr;

    Stats::Stats() : timers(9), level_stats(9, 0) {}

    Stats* Stats::GetInstance() {
        if (!singleton) singleton = new Stats();
        return singleton;
    }

    void Stats::StartTimer(uint32_t id) {
        for (size_t i = timers.size(); i < id + 1; ++i) timers.push_back(Timer{});

        Timer& timer = timers[id];
        timer.Start();
    }

    void Stats::PauseTimer(uint32_t id) {
        Timer& timer = timers[id];
        timer.Pause();
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
        for (size_t i = level_stats.size(); i < level + 1; ++i) level_stats.push_back(0);

        level_stats[level]++;
    }

    void Stats::ReportLevelStats() {
        for (int i = 0; i < level_stats.size(); ++i) {
            printf("Level %d: %u\n", i, level_stats[i]);
        }
    }

    void Stats::ResetAll() {
        timers.clear();
        level_stats.clear();
    }

    Stats::~Stats() {
        ReportTime();
    }














}
