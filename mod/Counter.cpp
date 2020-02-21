//
// Created by daiyi on 2020/02/12.
//

#include "Counter.h"


void Counter::Increment() {
    ++count;
}

void Counter::Reset() {
    count = 0;
}

int Counter::Report() {
    return count;
}