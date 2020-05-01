//
// Created by daiyi on 2020/02/12.
//

#include <iostream>
#include "Counter.h"


void Counter::Increment(int level, uint64_t n) {
    counts[level] += n;
}

void Counter::Reset() {
    for (uint64_t& count : counts) count = 0;
}

void Counter::Report() {
    std::cout << "Counter " << name << " " << Sum();
    for (uint64_t count : counts) {
        std::cout << " " << count;
    }
    std::cout << "\n";
}

int Counter::Sum() {
    return std::accumulate(counts.begin(), counts.end(), 0.0);
}