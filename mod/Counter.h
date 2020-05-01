//
// Created by daiyi on 2020/02/12.
//

#ifndef PROJECT1_COUNTER_H
#define PROJECT1_COUNTER_H

#include "../db/dbformat.h"
#include <vector>

class Counter {
    std::vector<uint64_t> counts;

public:
    std::string name;

    Counter() : counts(leveldb::config::kNumLevels + 1, 0) {};
    void Increment(int level, uint64_t n = 1);
    void Reset();
    void Report();
    int Sum();
};


#endif //PROJECT1_COUNTER_H
