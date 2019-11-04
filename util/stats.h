//
// Created by daiyi on 2019/09/30.
//

#ifndef LEVELDB_STATS_H
#define LEVELDB_STATS_H


#include <cstdint>
#include <map>
#include <vector>

namespace adgMod {
    extern bool MOD;

    class Timer;
    class Stats {

    private:
        static Stats* singleton;
        Stats();

        std::vector<Timer> timers;
        std::vector<uint32_t> level_stats;
    public:
        static Stats* GetInstance();
        void StartTimer(uint32_t id);
        void PauseTimer(uint32_t id);
        void ResetTimer(uint32_t id);
        uint64_t ReportTime(uint32_t id);
        void ReportTime();

        void RecordLevel(int level);
        void ReportLevelStats();

        void ResetAll();
        ~Stats();
    };





}


#endif //LEVELDB_STATS_H
