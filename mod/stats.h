//
// Created by daiyi on 2019/09/30.
//

#ifndef LEVELDB_STATS_H
#define LEVELDB_STATS_H


#include <cstdint>
#include <map>
#include <vector>
#include <cstring>
#include "timer.h"

using std::string;
using std::to_string;


namespace adgMod {

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
        void PauseTimer(uint32_t id, bool record = false);
        void ResetTimer(uint32_t id);
        uint64_t ReportTime(uint32_t id);
        void ReportTime();
        void ReportTimeSeries(uint32_t id);

        void RecordLevel(int level);
        void ReportLevelStats();

        void ResetAll(bool record = false);
        ~Stats();
    };


}


#endif //LEVELDB_STATS_H
