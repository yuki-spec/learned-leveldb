//
// Created by daiyi on 2020/02/12.
//

#ifndef PROJECT1_COUNTER_H
#define PROJECT1_COUNTER_H


class Counter {
    int count;

public:
    Counter() : count(0) {};
    void Increment();
    void Reset();
    int Report();
};


#endif //PROJECT1_COUNTER_H
