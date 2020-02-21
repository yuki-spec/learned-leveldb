//
// Created by daiyi on 2020/02/02.
//

#ifndef LEVELDB_TIMER_H
#define LEVELDB_TIMER_H


#include <cstdint>
#include <ctime>
#include <utility>
#include <vector>

namespace adgMod {

    class Timer {
        uint64_t time_started;
        uint64_t timestamp_accumulated;
        bool started;
        std::vector<std::pair<uint64_t, uint64_t>> time_series;

    public:
        void Start();
        void Pause(bool record = false);
        void Reset(bool record = false);
        uint64_t Time();
        void ReportTimeSeries();


        Timer();
        ~Timer() = default;
    };

}


#endif //LEVELDB_TIMER_H
